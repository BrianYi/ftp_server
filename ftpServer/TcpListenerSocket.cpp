#include "TcpListenerSocket.h"
#include "TcpSocket.h"
#include "EventHandler.h"
#include "Dispatcher.h"

TcpListenerSocket::TcpListenerSocket():
	Task(alive),
	TcpSocket()
{

}

TcpListenerSocket::~TcpListenerSocket( )
{

}

int32_t TcpListenerSocket::run( )
{
	// new connection
	if ( fEvents & EPOLLIN )
	{
		for(;; )
		{
			TcpSocket tcpSocket = this->accept( );
			if ( tcpSocket.fSocket == -1 )
				break;

			printf( "accept connection from %s:%d\n",
					tcpSocket.ip( ).c_str( ),
					tcpSocket.port( ) );

			TcpSession* tcpSession = new TcpSession( tcpSocket );
			sTcpSessionArry.push_back( tcpSession );

			struct epoll_event ev;
			ev.events = EPOLLIN | EPOLLET;
			ev.data.fd = tcpSocket.fSocket;

			EventHandler *handler = new EventHandler( ev, tcpSession );
			Dispatcher::register_handler( tcpSocket.fSocket, handler );
		}
	}

	return 0;
}

void TcpListenerSocket::request_event( int events )
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = this->fSocket;
	EventHandler *handler = new EventHandler( ev, this );
	Dispatcher::register_handler( this->fSocket, handler );
}

