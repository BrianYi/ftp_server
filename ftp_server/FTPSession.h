#pragma once
#include "ServerHeader.h"
#include "TcpSocket.h"
#include "Packet.h"
#include "Queue.h"
#include "Stack.h"

class DataTransferSession;

class FTPSession:
	public TcpSocket
{
public:
	enum
	{
		//
		UnAuthorized,
		Authorized,
		// 
	};
	FTPSession();
	FTPSession( int32_t fd, Address& address );
	FTPSession( const FTPSession& ) = delete;
	virtual ~FTPSession();
	virtual int32_t handle_event( uint32_t flags );
	void set_accept_time( int64_t acceptTime ) { fAcceptTime = acceptTime; }
	int64_t accept_time() { return fAcceptTime; }
	void push( Packet* ptrPacket )
	{
		fSendPriQueue.push( ptrPacket );
	}
// 	Packet* front()
// 	{
// 		return fSendPriQueue.front();
// 	}
	Packet* pop()
	{
		return fSendPriQueue.pop();
	}
	bool empty()
	{
		return fSendPriQueue.empty();
	}
	void set_current_dir( std::string dir ) { fCurDir = dir; }
	std::string current_dir() { return fCurDir; }
	void disconnect();
private:
	PriorityQueue<Packet*> fSendPriQueue;
	Stack<DataTransferSession*> fDataTransSessionStack;
	int64_t fAcceptTime;
	std::mutex fReadMx;
	std::mutex fWriteMx;
	uint32_t	fstate;
	std::string fCurDir;
	std::string fUserName;
	std::string fPassword;
};

