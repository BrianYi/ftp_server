#pragma once
#include "ServerHeader.h"
#include "TcpSocket.h"
#include "Packet.h"
#include "Queue.h"
#include "Stack.h"
#include <condition_variable>

class DataTransferSession;
class DataTransferListener;

class FTPSession:
	public TcpSocket
{
public:
	enum
	{
		//
		UnAuthorized,
		Authorized,
	};
	enum Mode
	{
		Active,
		Passive
	};
	FTPSession();
	FTPSession( int32_t fd );
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
	void push_data_transfer_session( DataTransferSession* session )
	{
		fDataTransSessionStack.push( session );
	}
private:
	PriorityQueue<Packet*> fSendPriQueue;
	Stack<DataTransferSession*> fDataTransSessionStack;
	DataTransferListener* fDataTransListener;
	int64_t fAcceptTime;
	std::mutex fReadMx;
	std::mutex fWriteMx;
	uint32_t	fstate;
	std::string fCurDir;
	std::string fUserName;
	std::string fPassword;
	Mode fMode;
};

