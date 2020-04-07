#include "TcpSocket.h"

TcpSocket::TcpSocket( ) :Socket( SOCK_STREAM, IPPROTO_TCP )
{

}

TcpSocket::TcpSocket( const TcpSocket& inTcpSocket ) : Socket( SOCK_STREAM, IPPROTO_TCP )
{
	*this = inTcpSocket;
}

// TCP::TCP( IOType inIOType ):Socket(SOCK_STREAM, inIOType)
// {
// 
// }

TcpSocket::~TcpSocket( )
{ }

void TcpSocket::listen( const uint16_t& inPort, const uint32_t& inQueueLength )
{
	if ( !this->fOpened )
		this->open( );

	if ( !this->fBinded )
	{
		Socket::bind_to_port( inPort );
	}

	if ( ::listen( this->fSocket, inQueueLength ) != 0 )
	{
		printf( "[TcpSocket] listen_on_port error!\n" );
		return;
	}
}

int32_t TcpSocket::connect( const Address& inAddress, IOType inIOType/* = Blocking*/, const time_t& timeout_ms/* = 1000*/ )
{
	if ( this->fBinded )
	{
		printf( "Socket %u already binded!\n", this->fSocket );
		return -1;
	}
	if ( !this->fOpened )
		this->open( );

	if ( this->fIOType != inIOType )
		this->setIOType( inIOType );

	if ( inIOType == Blocking )
		setsockopt( this->fSocket, SOL_SOCKET,
					SO_SNDTIMEO, ( char* ) &timeout_ms,
					sizeof( timeout_ms ) );

	int32_t ret = ::connect( this->fSocket, ( const sockaddr* ) &inAddress, sizeof( struct sockaddr ) );
	if ( ret == 0 )
	{
		this->fAddress = inAddress;
		this->fBinded = true;
	}
	return ret;
}

int32_t TcpSocket::connect( const std::string& inIP, const uint16_t& inPort, IOType inIOType/* = Blocking*/, const time_t& timeout_ms/* = 1000*/ )
{
	if ( this->fBinded )
	{
		printf( "Socket %u already binded!\n", this->fSocket );
		return -1;
	}
	if ( !this->fOpened )
		this->open( );

	if ( this->fIOType != inIOType )
		this->setIOType( inIOType );

	if ( inIOType == Blocking )
		setsockopt( this->fSocket, SOL_SOCKET,
					SO_SNDTIMEO, ( char* ) &timeout_ms,
					sizeof( timeout_ms ) );

	Address address( inIP, inPort );
	int32_t ret = ::connect( this->fSocket, ( const sockaddr* ) &address, sizeof( struct sockaddr ) );
	if ( ret == 0 )
	{
		this->fAddress = address;
		this->fBinded = true;
	}
	return ret;
}

TcpSocket TcpSocket::accept( IOType inIOType/* = Blocking*/, const time_t& timeout_ms/* = 1000*/ )
{
	if ( this->fIOType != inIOType )
		this->setIOType( inIOType );

	if ( inIOType == Blocking )
		setsockopt( this->fSocket, SOL_SOCKET,
					SO_SNDTIMEO, ( char* ) &timeout_ms,
					sizeof( timeout_ms ) );

	socklen_t size = sizeof( struct sockaddr );
	Address address;
	int socketID = ::accept( this->fSocket, ( struct sockaddr* )&address, &size );
	TcpSocket client;
	client.fSocket = socketID;
	client.fBinded = true;
	client.fOpened = true;
	client.fAddress = address;
	return client;
}

int32_t TcpSocket::send( const char* inContent, const size_t& inSize, IOType inIOType, const time_t& timeout_ms/* = 1000*/ )
{
	if ( !this->fOpened )
		this->open( );

	if ( inSize > SEND_BUF_SIZE )
	{
		printf( "Send buffer overflow!\n" );
		return -1;
	}

	if ( this->fIOType != inIOType )
		this->setIOType( inIOType );

	if ( inIOType == Blocking )
		setsockopt( this->fSocket, SOL_SOCKET,
					SO_SNDTIMEO, ( char* ) &timeout_ms,
					sizeof( timeout_ms ) );

	int32_t sentBytes = ::send( this->fSocket, inContent, inSize, 0 );
	return sentBytes;
}

int32_t TcpSocket::recv( char* outContent, const size_t& inSize, IOType inIOType, const time_t& timeout_ms/* = 1000*/ )
{
	if ( !this->fOpened )
		this->open( );
	if ( !this->fBinded )
	{
		printf( "Please first listen on port!\n" );
		return -1;
	}

	if ( this->fIOType != inIOType )
		this->setIOType( inIOType );

	if ( inIOType == Blocking )
		setsockopt( this->fSocket, SOL_SOCKET,
					SO_RCVTIMEO, ( char* ) &timeout_ms,
					sizeof( timeout_ms ) );

	int32_t receivedBytes = ::recv( this->fSocket, outContent, inSize, 0 );
	return receivedBytes;
}

Address TcpSocket::address( void )
{
	return fAddress;
}

std::string TcpSocket::ip( void )
{
	return fAddress.getIP( );
}

uint16_t TcpSocket::port( void )
{
	return fAddress.getPort( );
}
