#include "FTPSession.h"
#include "DataTransferSession.h"
#include "Utilities.h"


FTPSession::FTPSession():
	TcpSocket()
{
#if DEBUG_DataTransferSession
	printf( "new FTPSession %x\n", this );
#endif
	fAcceptTime = 0;
	fstate = UnAuthorized;
}

FTPSession::FTPSession( int32_t fd, Address& address ):
	TcpSocket( fd, true, address )
{
#if DEBUG_DataTransferSession
	printf( "new FTPSession %x\n", this );
#endif
	fAcceptTime = 0;
	fstate = UnAuthorized;
}

FTPSession::~FTPSession()
{
#if DEBUG_DataTransferSession
	printf( "del FTPSession %x\n", this );
#endif
}

int32_t FTPSession::handle_event( uint32_t flags )
{
	if ( fIsDead )
		return -1;

	char buf[MAX_BODY_SIZE];
	if ( flags & EPOLLIN )
	{
		// lock reading
		std::unique_lock<std::mutex> lockRead( this->fReadMx, std::try_to_lock );
		
		if ( !lockRead.owns_lock() )
			return 1;
		else
		{
#if DEBUG_FTPSession_RW_TIME
			DebugTime debugTime( DebugTime::Print, __LINEINFO__ );
#endif
			// this shot is triggered
			this->fEvents &= ~EPOLLIN;

			int32_t recvSize;
			for ( ;; )
			{
				recvSize = this->recv( buf, MAX_BODY_SIZE, Socket::NonBlocking );
				// peer connection lost
				if ( recvSize <= 0 )
				{
					if ( recvSize == 0 )
					{
						this->disconnect();
						return -1;
					}
					else // no data, recv buffer is empty
						break;
				}
				int len = recvSize - 2;
				buf[len] = 0;	// erase "\r\n"
				if ( buf[0] == '\362' )
				{
					memcpy( buf, &buf[1], len );
					--len;
				}
				// command interpreter
				std::vector<std::string> strArry = SplitString( buf, " " );
				int commandType = CommandType( strArry[0] );
				std::string retStr = "502 Command not implemented.\n";
				switch ( commandType )
				{
					case USER:
					{
						fUserName = strArry[1];
						retStr = "331 Password required for " + fUserName + "\n";
						break;
					}
					case PASS:
					{
						fPassword = strArry[1];
						if ( sys_auth_user( fUserName.c_str(), fPassword.c_str() ) != 0 )
						{
							retStr = "530 Login or password incorrect!\n";
							fstate = UnAuthorized;
						}
						else
						{
							retStr = "230 Logged on\n";
							fstate = Authorized;
						}
						break;
					}
					case QUIT:
					{
						// send goodbye
						retStr = "221 Goodbye!\n";
						this->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
						this->request_event( EPOLLOUT );

						// disconnect
						RTMP_LogAndPrintf( RTMP_LOGDEBUG, "lost connection from %s:%d",
							this->ip().c_str(),
							this->port() );
						this->kill_event();
						return -1;
					}
					default:
					{
						if ( fstate == UnAuthorized )
						{
							retStr = "530 Please log in with USER and PASS first.\n";
							break;
						}
						switch ( commandType )
						{
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
							case PORT:
							{
								std::vector<std::string> vecStr = SplitString( strArry[1], "," );
								std::string ipAddr = vecStr[0] + "." + vecStr[1] + "." + vecStr[2] + "." + vecStr[3];
								uint16_t dataPort = stoi( vecStr[4] ) * 256 + stoi( vecStr[5] );
								DataTransferSession *dataTransSesson = new DataTransferSession( this, fCurDir );
								dataTransSesson->set_accept_time( get_timestamp_ms() );
								if ( dataTransSesson->connect( ipAddr, dataPort, Socket::Blocking ) != 0 )
								{
									// failed
									retStr = "500 Cannot connect to " + ipAddr + ":" +
										std::to_string( dataPort ) + "\n";
									delete dataTransSesson;
									break;
								}

								// request read event
								//dataTransSesson->request_event( EPOLLIN );

								// success
								fDataTransSessionStack.push( dataTransSesson );
								retStr = "200 Port command successful\n";
								break;
							}
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
								// it must be 1
								assert( fDataTransSessionStack.size() == 1 );

								// success
								DataTransferSession *dataTransSession = fDataTransSessionStack.top();
								fDataTransSessionStack.pop();

								std::string filePath = strArry[1];
								(void)dataTransSession->exec_command( commandType, filePath );

								// prepare for sending data
								retStr = "150 Opening data channel for file download from server of '/" + filePath + "'\n";
								break;
							}
							case STOR:
							{
								// it must be 1
								assert( fDataTransSessionStack.size() == 1 );

								// success
								DataTransferSession *dataTransSession = fDataTransSessionStack.top();
								fDataTransSessionStack.pop();

								std::string filePath = strArry[1];
								(void)dataTransSession->exec_command( commandType, filePath );

								// prepare for receive data
								retStr = "150 Opening data channel for file upload to server of '/" + filePath + "'\n";
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
								// it must be 1
								assert( fDataTransSessionStack.size() == 1 );

								// success
								DataTransferSession *dataTransSession = fDataTransSessionStack.top();
								fDataTransSessionStack.pop();

								std::string dir = strArry.size() > 1 ? strArry[1] : "";
								(void)dataTransSession->exec_command( commandType, dir );

								// prepare for sending data
								retStr = "150 Opening data channel for file upload to server of '/" + dir + "'\n";
								break;
							}
							case NLIST:
								break;
							case SITE:
								break;
							case SYST:
							{
								retStr = "215 UNIX\n";
								break;
							}
							case STAT:
								break;
							case HELP:
								break;
							case NOOP:
								break;
							default:
								break;
						}
						break;
					}
				}
				this->push( PacketUtils::new_packet( commandType, retStr.c_str(), retStr.size(), 0 ) );
				this->request_event( EPOLLOUT );
			}
		}
	}
	if ( flags & EPOLLOUT )
	{
		// lock writing
		std::unique_lock<std::mutex> lockWrite( this->fWriteMx, std::try_to_lock );

		if ( !lockWrite.owns_lock() )
			return 1;
		else
		{
#if DEBUG_FTPSession_RW_TIME
			DebugTime debugTime( DebugTime::Print, __LINEINFO__ );
#endif
			// this shot is triggered
			this->fEvents &= ~EPOLLOUT;

			for ( ;; )
			{
				if ( this->empty() )
					break;

				Packet *ptrPacket = this->front();
				this->pop();
				int32_t sendSize = this->send( (char *)ptrPacket->body(), ptrPacket->body_size(), Socket::NonBlocking );

				if ( sendSize < 0 )
				{
					this->push( ptrPacket );
					if ( errno == EAGAIN )
						continue;
					else
						break;
				}

				delete ptrPacket;
			}
		}
	}
	return -1;
}

void FTPSession::disconnect()
{
	if ( dead() )
		return;

	// disconnect
	RTMP_LogAndPrintf( RTMP_LOGDEBUG, "lost connection from %s:%d",
		this->ip().c_str(),
		this->port() );

	/*
	 * we don't need to delete DataTransferSession
	 * it would delete by itself
	 */
	this->kill_event();
}
