#include "TcpSession.h"
#include "Dispatcher.h"

TcpSession::TcpSession( TcpSocket tcpSocket ):
	Task(alive ),
	TcpSocket(tcpSocket)
{

}

int32_t TcpSession::run( )
{
	char buf[ 512 ];
	if ( fEvents & EPOLLIN )
	{
		static int32_t totalSize = 0;
		for(;; )
		{
			int32_t recvSize = recv( buf, sizeof buf );
			if ( recvSize == -1 )
				break;
			else if ( recvSize == 0 )
			{
				printf( "lost connection from %s:%d(fd=%d)\n", 
						this->ip( ).c_str( ), 
						this->port( ),
						this->fSocket );

				//Dispatcher::remove_handler( this->fSocket );
				break;
			}
			buf[ recvSize ] = '\0';
			totalSize += recvSize;
			printf( "recv from %s:%d(fd=%d) %dB, total=%dB, %s\n",
					this->ip( ).c_str( ),
					this->port( ),
					this->fSocket,
					recvSize,
					totalSize,
					buf );
		}
	}
	else if ( fEvents & EPOLLOUT )
	{
		sprintf( buf, "hello world!" );
		int32_t sendSize = send( buf, sizeof buf );
		printf( "send to %s:%d %dB\n",
				this->ip( ).c_str( ),
				this->port( ),
				sendSize);
	}
	return 0;
}
