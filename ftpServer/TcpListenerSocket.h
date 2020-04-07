#pragma once
#include "TcpSocket.h"
#include "Task.h"
#include "TcpSession.h"
#include <vector>

class TcpListenerSocket :
	public TcpSocket, Task
{
public:
	TcpListenerSocket();
	~TcpListenerSocket( );
	virtual int32_t run( );
	virtual void request_event( uint32_t events );
protected:
	std::vector<TcpSession*> sTcpSessionArry;
	//void request_event( int events );
};

