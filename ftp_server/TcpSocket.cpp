/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#include "TcpSocket.h"
#include "ServerHeader.h"

TcpSocket::TcpSocket( ) :
	EventHandler( SOCK_STREAM, IPPROTO_TCP, Blocking )
{

}

// TcpSocket::TcpSocket( const TcpSocket& inTcpSocket ) : EventHandler( SOCK_STREAM, IPPROTO_TCP, Blocking )
// {
// 	*this = inTcpSocket;
// }

TcpSocket::TcpSocket( int32_t fd, bool binded ) : 
	EventHandler( fd, binded, SOCK_STREAM, IPPROTO_TCP, Blocking )
{
	//fAddress = address;
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

int32_t TcpSocket::connect( const Address& inAddress, IOType inIOType )
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

	int32_t ret = ::connect( this->fSocket, ( const sockaddr* ) &inAddress, sizeof( struct sockaddr ) );
	if ( ret == 0 )
	{
		//this->fAddress = inAddress;
		this->fBinded = true;
	}
	return ret;
}

int32_t TcpSocket::connect( const std::string& inIP, const uint16_t& inPort, IOType inIOType )
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

	Address address( inIP, inPort );
	int32_t ret = ::connect( this->fSocket, ( const sockaddr* ) &address, sizeof( struct sockaddr ) );
	if ( ret < 0 )
	{
		printf( "Connect error: %d\n", errno );
		return ret;
	}
	//this->fAddress = address;
	this->fBinded = true;
	return ret;
}

TcpSocket* TcpSocket::accept( IOType inIOType )
{
	if ( this->fIOType != inIOType )
		this->setIOType( inIOType );

	socklen_t size = sizeof( struct sockaddr );
	Address address;
	int socketID = ::accept( this->fSocket, ( struct sockaddr* )&address, &size );
	
	if ( socketID == -1 ) return nullptr;

	TcpSocket* ptrTcpSocket = new TcpSocket;
	ptrTcpSocket->fSocket = socketID;
	ptrTcpSocket->fBinded = true;
	ptrTcpSocket->fOpened = true;
	//ptrTcpSocket->fAddress = address;
	return ptrTcpSocket;
}

int32_t TcpSocket::send( const char* inContent, const uint32_t& inSize, IOType inIOType )
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

	int32_t sentBytes = ::send( this->fSocket, inContent, inSize, 0 );
	return sentBytes;
}

int32_t TcpSocket::recv( char* outContent, const uint32_t& inSize, IOType inIOType )
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

	int32_t receivedBytes = ::recv( this->fSocket, outContent, inSize, 0 );
	return receivedBytes;
}

Address TcpSocket::address( void )
{
	Address address;
	socklen_t len = sizeof( struct sockaddr );
	getpeername( this->fSocket, (sockaddr*)&address, &len );
	return address;
}

std::string TcpSocket::ip( void )
{
	Address address;
	socklen_t len = sizeof( struct sockaddr );
	getpeername( this->fSocket, (sockaddr*)&address, &len );
	return address.ip( );
}

uint16_t TcpSocket::port( void )
{
	Address address;
	socklen_t len = sizeof( struct sockaddr );
	getpeername( this->fSocket, (sockaddr*)&address, &len );
	return address.port( );
}

uint16_t TcpSocket::local_port( void )
{
	Address address;
	socklen_t len = sizeof( struct sockaddr );
	getsockname( this->fSocket, (sockaddr*)&address, &len );
	return address.port();
}
