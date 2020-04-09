#include "RtmpListenerSocket.h"
#include "RtmpSession.h"
#include "RtmpSessionTable.h"

RtmpListenerSocket::RtmpListenerSocket( )
{

}

RtmpListenerSocket::~RtmpListenerSocket( )
{

}

int32_t RtmpListenerSocket::handle_event( uint32_t flags )
{
	// new connection
	if ( flags & EPOLLIN )
	{
		// lock reading
		std::unique_lock<std::mutex> lockRead( fReadMx );

		// this shot is triggered
		//this->fEvents &= ~EPOLLIN;

		for ( ;; )
		{
			RtmpSession* ptrRtmpSession = this->accept( NonBlocking );
			if ( ptrRtmpSession == nullptr )
				break;

			RTMP_LogAndPrintf( RTMP_LOGDEBUG, "accept connection from %s:%d",
					ptrRtmpSession->ip( ).c_str( ),
					ptrRtmpSession->port( ) );

			ptrRtmpSession->set_accept_time( get_timestamp_ms( ) );

			// request read event
			ptrRtmpSession->request_event( EPOLLIN | EPOLLOUT );
		}

		// request new connect event
		//this->request_event( EPOLLIN );
	}
	return 0;
}

RtmpSession* RtmpListenerSocket::accept( IOType inIOType /*= Blocking */ )
{
	if ( this->fIOType != inIOType )
		this->setIOType( inIOType );

	socklen_t size = sizeof( struct sockaddr );
	Address address;
	int socketID = ::accept( this->fSocket, ( struct sockaddr* )&address, &size );

	if ( socketID == -1 ) return nullptr;

	RtmpSession* ptrRtmpSession = new RtmpSession( socketID, address );
	return ptrRtmpSession;
}
