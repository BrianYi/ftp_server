#include "DataTransferSession.h"
#include "Utilities.h"
#include "Dispatcher.h"

DataTransferSession::DataTransferSession( FTPSession *ftpSession, std::string currentDir ) :
	TcpSocket()
{
	fAcceptTime = 0;
	fFTPSession = ftpSession;
	fCurrentDir = currentDir;
	fReadFinished = false;
	//fstate = UnAuthorized;
}

DataTransferSession::DataTransferSession( FTPSession *ftpSession, std::string currentDir, int32_t fd, Address& address ) :
	TcpSocket( fd, true, address )
{
	fAcceptTime = 0;
	fFTPSession = ftpSession;
	fCurrentDir = currentDir;
	fReadFinished = false;
	//fstate = UnAuthorized;
}

DataTransferSession::~DataTransferSession()
{

}

int32_t DataTransferSession::handle_event( uint32_t flags )
{
	char buf[MAX_BODY_SIZE];
	if ( flags & EPOLLIN )
	{
		// lock reading
		std::unique_lock<std::mutex> lockRead( this->fReadMx );

		// this shot is triggered
		this->fEvents &= ~EPOLLIN;

		// if it still in killed
		if ( fIsDead || fReadFinished )
			return 0;

		/*
		 * judge if can we write to file?
		 */
		std::string relPath = "/" + fFilePath;
		std::string absFilePath = fCurrentDir + relPath;
		
		int fd = -1;
		if ( access( absFilePath.c_str(), F_OK ) != -1 )
			fd = ::open( absFilePath.c_str(), O_WRONLY | O_TRUNC | O_EXCL );
		else
			fd = ::open( absFilePath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0777 );

		if ( fd == -1 )
		{
			std::string retStr =
				"553-Requested action not taken.\n"
				"553 File name not allowed.\n";
			fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
			fFTPSession->request_event( EPOLLOUT );

			this->kill_event();
			return -1;
		}
		::close( fd );
		
		/*
		 * receive file data
		 */
		int32_t recvSize;
		for ( ;; )
		{
			recvSize = this->recv( buf, MAX_BODY_SIZE, Socket::Blocking );
			// peer connection lost
			if ( recvSize <= 0 )
			{
				if ( recvSize == 0 ) // lost connection
				{
					fReadFinished = true;
					this->push( PacketUtils::new_packet( STOR, buf, recvSize, 0 ) );
					Task *task = new Task( this, EPOLLOUT );
					Dispatcher::push_to_thread( task );
					return -1;
				}
				else // <0 is recv buffer is empty, EAGAIN
				{
					assert( false );
// 					std::string retStr = "426 Connection closed; transfer aborted.\n";
// 					fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0) );
// 					fFTPSession->request_event( EPOLLOUT );
// 
// 					// close file
// 					::close( fFileDesc );

					return -1;
				}
			}
			else
			{
				this->push( PacketUtils::new_packet( STOR, buf, recvSize, 1 ) );
				Task *task = new Task( this, EPOLLOUT );
				Dispatcher::push_to_thread( task );
			}

		}
	}

	if ( flags & EPOLLOUT )
	{
		/*
		 * RETR file
		 */

		 // lock writing
		std::unique_lock<std::mutex> lockWrite( this->fWriteMx );

		// this shot is triggered
		this->fEvents &= ~EPOLLOUT;

		// if it still in killed
		if ( fIsDead )
			return 0;


		std::string relPath = "/" + fFilePath;
		std::string absFilePath = fCurrentDir + relPath;
		bool isOpenedFile = false;
		int fd = -1;
		for ( ;; )
		{
			if ( this->empty() )
			{
				msleep( 1 );
				continue;
			}

			Packet *ptrPacket = this->front();
			this->pop();

			// last packet is the message need to send to control port
			if ( !ptrPacket->more() )
			{
				switch (ptrPacket->type() )
				{
				case RETR:
				case LIST:
				{
					// finished sending file
					std::string retStr = "226 Successfully transferred '" +
						std::string( ptrPacket->body() ) + "'\n";
					fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
					fFTPSession->request_event( EPOLLOUT );
					this->kill_event();
					delete ptrPacket;
					return 0;
				}
				case STOR:
				{
					// finished write to file
					std::string retStr = "226 Successfully transferred '" + relPath + "'\n";
					fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
					fFTPSession->request_event( EPOLLOUT );

					// close file
					::close( fd );

					// kill event
					this->kill_event();
					return 0;
				}
				default:
					assert( false );
					break;
				}
			}
			
			// send or write packet
			if ( ptrPacket->type() == STOR )
			{
				if ( !isOpenedFile )
				{
					if ( access( absFilePath.c_str(), F_OK ) != -1 )
						fd = ::open( absFilePath.c_str(), O_WRONLY | O_TRUNC | O_EXCL );
					else
						fd = ::open( absFilePath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0777 );
					isOpenedFile = true;
					assert( fd != -1 );
				}
				// save data
				ssize_t writeSize = write( fd, ptrPacket->body(), ptrPacket->body_size() );
				if ( writeSize == -1 )
					printf( "errno=%d\n", errno );
				assert( writeSize == ptrPacket->body_size() );
			}
			else
			{
				int sendSize = this->send( ptrPacket->body(), ptrPacket->body_size(), Socket::NonBlocking );
				if ( sendSize < 0 )
				{
					this->push( ptrPacket );
					if ( errno == EAGAIN )
						continue;
					else
						assert( false );
				}
			}

			delete ptrPacket;
		}
		printf( "Lock out %d\n", this );
	}

	return 0;
}

int DataTransferSession::exec_command( int32_t commandType, std::string params )
{
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
		char buf[MAX_BODY_SIZE];

		// open file
		std::string relPath = "/" + params;
		std::string absPath = fCurrentDir + relPath;
		int fd = ::open( absPath.c_str(), O_RDONLY );
		if ( fd == -1 )
		{
			std::string retStr =
				"553 Requested action not taken.\n"
				"553 File name not allowed.\n";
			fFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
			fFTPSession->request_event( EPOLLOUT );

			this->kill_event();
			return -1;
		}

		// read file
		for ( ;; )
		{
			int readSize = read( fd, buf, MAX_BODY_SIZE );
			if ( readSize <= 0 )
			{
				// finished
				::close( fd );

				this->push( PacketUtils::new_packet( commandType, relPath.c_str(), relPath.size() + 1, 0 ) );
				this->request_event( EPOLLOUT );
				break;
			}
			else
			{
				this->push( PacketUtils::new_packet( commandType, buf, readSize, 1 ) );
				this->request_event( EPOLLOUT );
			}
		}
		break;
	}
	case STOR:
	{
		fFilePath = params;
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
		this->push( PacketUtils::new_packet( commandType, relDir.c_str(), relDir.size()+1, 0 ) );
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

