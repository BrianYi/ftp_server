/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#include "FTPSession.h"
#include "DataTransferSession.h"
#include "Utilities.h"
#include "DataTransferListener.h"


FTPSession::FTPSession():
	TcpSocket()
{
#if DEBUG_DataTransferSession
	printf( "new FTPSession %x\n", this );
#endif
	fAcceptTime = 0;
	fstate = UnAuthorized;
	fMode = Active;
}

FTPSession::FTPSession( int32_t fd ):
	TcpSocket( fd, true )
{
#if DEBUG_DataTransferSession
	printf( "new FTPSession %x\n", this );
#endif
	fAcceptTime = 0;
	fstate = UnAuthorized;
	fMode = Active;
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
#if DEBUG_FTPSession_RD_TIME
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
				std::string retStr = "502 Command not implemented.\r\n";
				switch ( commandType )
				{
					case USER:
					{
						fUserName = strArry[1];
						retStr = "331 Password required for " + fUserName + "\r\n";
						break;
					}
					case PASS:
					{
						fPassword = strArry[1];
						if ( sys_auth_user( fUserName.c_str(), fPassword.c_str() ) != 0 )
						{
							retStr = "530 Login or password incorrect!\r\n";
							fstate = UnAuthorized;
						}
						else
						{
							retStr = "230 Logged on\r\n";
							fstate = Authorized;
						}
						break;
					}
					case QUIT:
					{
						// send goodbye
						retStr = "221 Goodbye!\r\n";
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
							retStr = "530 Please log in with USER and PASS first.\r\n";
							break;
						}

						/*
						 * Only if password matched with username
						 * then will get here
						 */
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
								/*
								 * if C send PORT to S, it should be active mode
								 * so S need to connect to C
								 * example: 
								 * if S received PORT 192,168,1,100,12,34
								 * it means ip of peer connection C is 192.168.1.100, with it's port
								 * is 12<<8+34=12*256+34=3106, so S should connect to 192.168.1.100:3106
								 * port is unsigned short(2B)
								 */
								std::vector<std::string> vecStr = SplitString( strArry[1], "," );
								std::string ipAddr = vecStr[0] + "." + vecStr[1] + "." + vecStr[2] + "." + vecStr[3];
								uint16_t dataPort = stoi( vecStr[4] ) * 256 + stoi( vecStr[5] );
								DataTransferSession *dataTransSession = new DataTransferSession( this, fCurDir );
								dataTransSession->set_accept_time( get_timestamp_ms() );
								if ( dataTransSession->connect( ipAddr, dataPort, Socket::Blocking ) != 0 )
								{
									// get failed, sending response error
									retStr = "500 Cannot connect to " + ipAddr + ":" +
										std::to_string( dataPort ) + "\r\n";
									delete dataTransSession;
									break;
								}

								// success, sending response
								fDataTransSessionStack.push( dataTransSession );
								retStr = "200 Port command successful\r\n";
								break;
							}
							case PASV:
							{
								fMode = Passive;
								/*
								 * dataTransListener will kill itself when it successful received an DataTransferSession
								 * if it always in waiting queue, and client has already send STOR/RETR in advance, it
								 * could cause an error!
								 * already fixed, we don't push it into task queue, we just use it directly in next turn
								 * in the next turn, call handle_accept directly to accept C's request
								 */
								fDataTransListener = new DataTransferListener( this, fCurDir );
								
								/*
								 * in passive mode, we need to open a port to accept C's connect request
								 * we pass 0 port into listen, it would find an usable port automatically 
								 * and listen it
								 */
								fDataTransListener->listen( 0, 64 );
								
								/*
								 * local_port would return local host port
								 * we response S's ip and our newly opened listening port like below:
								 * 227 Entering Passive Mode (192,168,1,107,34,56)
								 * it means we opened a port 34<<8+56=34*256+56=8760 for listening
								 * when C connected to DataTransferListener, it would spawn a new socket by accept
								 * and we then send or receive data by this socket
								 * DataTransferListener will kill itself later
								 */
								uint16_t port = fDataTransListener->local_port();
								std::vector<std::string> vecStr = SplitString( SERVER_IP, "." );
								retStr = "227 Entering Passive Mode (" +
									vecStr[0] + "," + vecStr[1] + "," + vecStr[2] + "," + vecStr[3] + "," +
									std::to_string( port >> 8 ) + "," + std::to_string( port & 0xff ) +
									").\r\n";

#if 0
								RTMP_LogAndPrintf( RTMP_LOGDEBUG, "(response)PASV FTPSession=%x, dataTransSession=%x %s",
									this, dataTransSession, retStr.c_str() );
#endif
								break;
							}
							case TYPE:
								break;
							case STRU:
								break;
							case MODE:
								break;
							case RETR:
							{
								if ( fMode == Passive )
								{
									if ( fDataTransSessionStack.empty() )
									{
										fDataTransListener->handle_accept( );
										delete fDataTransListener;
									}
								}
								// it must be 1
								assert( fDataTransSessionStack.size() == 1 );

								// success
								DataTransferSession *dataTransSession = fDataTransSessionStack.pop();

								std::string filePath = strArry[1];
								(void)dataTransSession->exec_command( commandType, filePath );

								// prepare for sending data
								retStr = "150 Opening data channel for file download from server of '/" + filePath + "'\r\n";
								break;
							}
							case STOR:
							{
								/*
								 * if we are at passive mode, then before we dealing with any
								 * operation command, we need reassure our data connect has already
								 * established with C by check fDataTransSessionStack's size
								 * if it is zero, means we haven't deal with C's connect request
								 * so we just call handle_accept() to deal with request
								 * then data connection established
								 * we can sending or receiving data now
								 */
								if ( fMode == Passive )
								{
									if ( fDataTransSessionStack.empty() )
									{
										fDataTransListener->handle_accept();
										delete fDataTransListener;
									}
								}
								// it must be 1
								assert( fDataTransSessionStack.size() == 1 );

								// success
								DataTransferSession *dataTransSession = fDataTransSessionStack.pop();

								std::string filePath = strArry[1];
								(void)dataTransSession->exec_command( commandType, filePath );

								// prepare for receive data
								retStr = "150 Opening data channel for file upload to server of '/" + filePath + "'\r\n";
#if 0
								RTMP_LogAndPrintf( RTMP_LOGDEBUG, "(response)STOR FTPSession=%x, dataTransSession=%x %s",
									this, dataTransSession, retStr.c_str() );
#endif
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
								while ( fDataTransSessionStack.empty() )
									::usleep( 1 );
								// it must be 1
								assert( fDataTransSessionStack.size() == 1 );

								// success
								DataTransferSession *dataTransSession = fDataTransSessionStack.pop();

								std::string dir = strArry.size() > 1 ? strArry[1] : "";
								(void)dataTransSession->exec_command( commandType, dir );

								// prepare for sending data
								retStr = "150 Opening data channel for file upload to server of '/" + dir + "'\r\n";
								break;
							}
							case NLIST:
								break;
							case SITE:
								break;
							case SYST:
							{
								retStr = "215 UNIX\r\n";
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
#if DEBUG_FTPSession_WR_TIME
			DebugTime debugTime( DebugTime::Print, __LINEINFO__ );
#endif
			// this shot is triggered
			this->fEvents &= ~EPOLLOUT;

			for ( ;; )
			{
				if ( this->empty() )
					break;

				Packet *ptrPacket = this->pop();
				//this->pop();
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
