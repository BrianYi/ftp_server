#include "DataTransferSession.h"
#include "Utilities.h"
#include "Dispatcher.h"

DataTransferSession::DataTransferSession( FTPSession *ftpSession, std::string currentDir ) :
	TcpSocket()
{
#if DEBUG_DataTransferSession
	printf( "new DataTransferSession %x\n", this );
#endif
	fAcceptTime = 0;
	fFTPSession = ftpSession;
	fCurrentDir = currentDir;
	fRcvFinished = false;
	fFileDesc = -1;
	fType = UNKNOWN;
	//fstate = UnAuthorized;
}

DataTransferSession::DataTransferSession( FTPSession *ftpSession, std::string currentDir, int32_t fd, Address& address ) :
	TcpSocket( fd, true, address )
{
#if DEBUG_DataTransferSession
	printf( "new DataTransferSession %x\n", this );
#endif
	fAcceptTime = 0;
	fFTPSession = ftpSession;
	fCurrentDir = currentDir;
	fRcvFinished = false;
	fFileDesc = -1;
	fType = UNKNOWN;
	//fstate = UnAuthorized;
}

DataTransferSession::~DataTransferSession()
{
#if DEBUG_DataTransferSession
	printf( "del DataTransferSession %x\n", this );
#endif
}

int32_t DataTransferSession::handle_event( uint32_t flags )
{
	// if it still in killed
	if ( fIsDead )
		return -1;

	// kill by itself
	if ( fFTPSession->dead() )
	{
		this->kill_event();
		return -1;
	}

	if ( flags & EPOLLIN )
	{
		// lock reading
		std::unique_lock<std::timed_mutex> lockRead( this->fReadMx, std::defer_lock );
		if ( lockRead.try_lock_for() )
		{
#if DEBUG_DataTransferSession_RW_TIME
			DebugTime debugTime( DebugTime::Print, __LINEINFO__ );
#endif
			// this shot is triggered
			this->fEvents &= ~EPOLLIN;

			if ( fRcvFinished )
				return -1;

			std::string relFilePath = "/" + fFilePath;
			std::string absFilePath = fCurrentDir + relFilePath;
			for ( ;; )
			{
				if ( fType == STOR )
				{
					const int blockSize = 10 * MAX_BODY_SIZE;
					char recvBuf[blockSize];
					int recvSize = this->recv( recvBuf, blockSize, Socket::NonBlocking );
					// peer connection lost
					if ( recvSize <= 0 )
					{
						if ( recvSize == 0 ) // lost connection or finished
						{
							fRcvFinished = true;
							this->push( PacketUtils::new_packet( fType, recvBuf, recvSize, 0 ) );
							Task *task = new Task( this, EPOLLOUT );
							Dispatcher::push_to_thread( task );
							break;
						}
						else // EAGAIN
						{
							if ( errno == EAGAIN )
							{
								this->request_event( EPOLLIN );
								break;
							}
							else
								assert( false );
						}
					}
					else
					{
						int32_t numPack = (recvSize + MAX_BODY_SIZE - 1) / MAX_BODY_SIZE;
						int32_t bodySize = 0;
						for ( int i = 0; i < numPack; ++i )
						{
							bodySize = (i == numPack - 1) ?
								recvSize - i * MAX_BODY_SIZE : MAX_BODY_SIZE;
							this->push( PacketUtils::new_packet( fType, recvBuf + i * MAX_BODY_SIZE, bodySize, 1 ) );
							Task *task = new Task( this, EPOLLOUT );
							Dispatcher::push_to_thread( task );
						}
					}
				}
				else if ( fType == RETR)
				{
					// judge if file already opened
					// in multi-thread environment, one thread opened file, others just read
					if ( fFileDesc == -1 )
					{
						fFileDesc = ::open( absFilePath.c_str(), O_RDONLY );
						if ( fFileDesc == -1 )
						{
							std::string retStr =
								"450-Requested file action not taken.\n"
								"450 File doesn't exist.\n";
							fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
							fFTPSession->request_event( EPOLLOUT );
							this->kill_event();
							return -1;
						}
					}


					const int blockSize = MAX_BODY_SIZE;
					char readBuf[blockSize];
					int readSize = 0;
					readSize = read( fFileDesc, readBuf, blockSize );
					if ( readSize <= 0 )
					{
						// finished
						::close( fFileDesc );

						this->push( PacketUtils::new_packet( fType, relFilePath.c_str(), relFilePath.size() + 1, 0 ) );
						this->request_event( EPOLLOUT );
					}
					else
					{
						int32_t numPack = (readSize + MAX_BODY_SIZE - 1) / MAX_BODY_SIZE;
						int32_t bodySize = 0;
						for ( int i = 0; i < numPack; ++i )
						{
							bodySize = (i == numPack - 1) ?
								readSize - i * MAX_BODY_SIZE : MAX_BODY_SIZE;
							this->push( PacketUtils::new_packet( fType, readBuf + i * MAX_BODY_SIZE, bodySize, 1 ) );
						}
						this->request_event( EPOLLOUT );
					}
					break;
				}
				else assert( false );
			}
		}
	}

	if ( flags & EPOLLOUT )
	{
		// lock writing
		std::unique_lock<std::timed_mutex> lockWrite( this->fWriteMx, std::defer_lock );
		if ( lockWrite.try_lock( /*std::chrono::milliseconds( 10 )*/ ) )
		{
#if DEBUG_DataTransferSession_RW_TIME
			DebugTime debugTime( DebugTime::Print, __LINEINFO__ );
#endif
			// this shot is triggered
			this->fEvents &= ~EPOLLOUT;

			// if it still in killed
// 			if ( fIsDead )
// 				return 0;

			std::string relFilePath = "/" + fFilePath;
			std::string absFilePath = fCurrentDir + relFilePath;

			for ( ;; )
			{
				if ( this->empty() )
				{
					break;
				}

				Packet *ptrPacket = this->front();
				this->pop();

				if ( fType == STOR )
				{
					// judge if store finished?(last packet is useless,just a clue of finishing)
					if ( !ptrPacket->more() )
					{
						// finished write to file
						std::string retStr = "226 Successfully upload '" + relFilePath + "'\n";
						fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
						fFTPSession->request_event( EPOLLOUT );

						// close file
						::close( fFileDesc );

						// kill event
						this->kill_event();
						return -1;
					}

					// judge if file already opened
					// in multi-thread environment, one thread opened file, others just write
					if ( fFileDesc == -1 )
					{
						if ( access( absFilePath.c_str(), F_OK ) != -1 )
							fFileDesc = ::open( absFilePath.c_str(), O_WRONLY | O_TRUNC | O_EXCL );
						else
							fFileDesc = ::open( absFilePath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0777 );
						if ( fFileDesc == -1 )
						{
							std::string retStr = "450 Requested file action not taken.\n";
							fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
							fFTPSession->request_event( EPOLLOUT );
							this->kill_event();
							return -1;
						}
					}

					// write data
					ssize_t writeSize = write( fFileDesc, ptrPacket->body(), ptrPacket->body_size() );
					assert( writeSize == ptrPacket->body_size() );
				}
				else if ( fType == RETR )
				{
					if ( !ptrPacket->more() )
					{
						// finished sending file
						std::string retStr = "226 Successfully download '" +
							std::string( ptrPacket->body() ) + "'\n";
						fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
						fFTPSession->request_event( EPOLLOUT );
						this->kill_event();
						delete ptrPacket;
						return -1;
					}

					int sendSize = this->send( ptrPacket->body(), ptrPacket->body_size(), Socket::NonBlocking );
					if ( sendSize < 0 )
					{
						if ( this->dead() ) // is closed connection?
							return -1;
						else
						{
							this->push( ptrPacket );
							if ( errno == EAGAIN )
								continue;
							else
								assert( false );
						}
					}
					else
					{
						// signal to read more packets
						if ( this->empty() )
						{
							Task *task = new Task( this, EPOLLIN );
							Dispatcher::push_to_thread( task );
							return -1;
						}
					}
					break;
				}
				else if ( fType == LIST )
				{

					if ( !ptrPacket->more() )
					{
						// finished sending file
						std::string retStr = "226 Successfully download '" +
							std::string( ptrPacket->body() ) + "'\n";
						fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
						fFTPSession->request_event( EPOLLOUT );
						this->kill_event();
						delete ptrPacket;
						return -1;
					}

					int sendSize = this->send( ptrPacket->body(), ptrPacket->body_size(), Socket::NonBlocking );
					if ( sendSize < 0 )
					{
						if ( this->dead() ) // is closed connection?
							return -1;
						else
						{
							this->push( ptrPacket );
							if ( errno == EAGAIN )
								continue;
							else
								assert( false );
						}
					}
				}

				delete ptrPacket;
			}
		}
	}

	return -1;
}

int DataTransferSession::exec_command( int32_t commandType, std::string params )
{
	fType = commandType;
	switch ( commandType )
	{
		case USER:
			break;
		case PASS:
			break;
		case ACCT:
			break;
		case CWD:
			break;
		case CDUP:
			break;
		case SMNT:
			break;
		case REIN:
			break;
		case QUIT:
			break;
		case PORT:
			break;
		case PASV:
			break;
		case TYPE:
			break;
		case STRU:
			break;
		case MODE:
			break;
		case RETR:
		{
			fFilePath = params;
			Task *task = new Task( this, EPOLLIN );
			Dispatcher::push_to_thread( task );
			break;
		}
		case STOR:
		{
			fFilePath = params;
			this->request_event( EPOLLIN );
			break;
		}
		case STOU:
			break;
		case APPE:
			break;
		case ALLO:
			break;
		case REST:
			break;
		case RNFR:
			break;
		case RNTO:
			break;
		case ABOR:
			break;
		case DELE:
			break;
		case RMD:
			break;
		case MKD:
			break;
		case PWD:
			break;
		case LIST:
		{
			std::string relDir = "/" + params;
			std::string absDir = fCurrentDir + relDir;
			std::string output = ls_command( absDir );
			const char *p = output.c_str();
			int32_t numPack = (output.size() + MAX_BODY_SIZE - 1) / MAX_BODY_SIZE;
			int32_t bodySize = 0;
			Packet *ptrPacket = NULL;
			for ( int i = 0; i < numPack; ++i )
			{
				if ( i == numPack - 1 )
					bodySize = output.size() - i * MAX_BODY_SIZE;
				else
					bodySize = MAX_BODY_SIZE;
				ptrPacket = PacketUtils::new_packet( commandType, p + i * MAX_BODY_SIZE, bodySize, 1 );
				this->push( ptrPacket );
			}
			this->push( PacketUtils::new_packet( commandType, relDir.c_str(), relDir.size() + 1, 0 ) );
			this->request_event( EPOLLOUT );
			break;
		}
		case NLIST:
			break;
		case SITE:
			break;
		case SYST:
			break;
		case STAT:
			break;
		case HELP:
			break;
		case NOOP:
			break;
		case UNKNOWN:
			break;
		default:
			break;
	}
	return 0;
}

