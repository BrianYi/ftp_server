/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#include "FTPListener.h"
#include "FTPSession.h"
#include "Log.h"

FTPListener::FTPListener( std::string ftpHome ):
	TcpSocket()
{
	fFtpHome = ftpHome;
}

FTPListener::~FTPListener()
{

}

int32_t FTPListener::handle_event( uint32_t flags )
{
	// if it still in killed
	if ( fIsDead )
		return -1;

	// new connection
	if ( flags & EPOLLIN )
	{
		// lock reading
		std::unique_lock<std::mutex> lockRead( fReadMx );

		if ( !lockRead.owns_lock() )
			return 1;
		else
		{
			// this shot is triggered
			this->fEvents &= ~EPOLLIN;

			for ( ;; )
			{
				FTPSession* ptrFTPSession = this->accept( NonBlocking );
				if ( ptrFTPSession == nullptr )
					break;

				RTMP_LogAndPrintf( RTMP_LOGDEBUG, "accept connection from %s:%d",
					ptrFTPSession->ip().c_str(),
					ptrFTPSession->port() );

				ptrFTPSession->set_current_dir( fFtpHome );
				ptrFTPSession->set_accept_time( get_timestamp_ms() );

				// send out reply
				std::string retStr = "220-Service ready for new user.\r\n"
					"220-FTP Sever " SERVER_VERSION "\r\n"
					"220-written by BrianYi (406787679@qq.com)\r\n"
					"220 welcome to my github https://github.com/brianyi \r\n";
				ptrFTPSession->push( PacketUtils::new_packet( REPLY, retStr.c_str(), retStr.size(), 0 ) );
				ptrFTPSession->request_event( EPOLLOUT );
			}

			// request new connect event
			this->request_event( EPOLLIN );
		}
	}
	return -1;
}

FTPSession* FTPListener::accept( IOType inIOType )
{
	if ( this->fIOType != inIOType )
		this->setIOType( inIOType );

	socklen_t len = sizeof( struct sockaddr );
	Address address;
	int socketID = ::accept( this->fSocket, (struct sockaddr*)&address, &len );

	if ( socketID == -1 ) return nullptr;

	FTPSession* ptrFTPSession = new FTPSession( socketID );
	return ptrFTPSession;
}
