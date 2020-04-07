#pragma once
#include "Task.h"
#include "TcpSocket.h"
class TcpSession :
	public Task, TcpSocket
{ 
public:
	TcpSession( TcpSocket tcpSocket );
	virtual int32_t run( );
};

