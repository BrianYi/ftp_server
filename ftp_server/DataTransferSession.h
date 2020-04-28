#pragma once
#include "TcpSocket.h"
#include "Queue.h"
#include "Packet.h"
#include "FTPSession.h"
class DataTransferSession :
	public TcpSocket
{
public:
	DataTransferSession( FTPSession *ftpSession, std::string currentDir );
	DataTransferSession( FTPSession *ftpSession, std::string currentDir, int32_t fd, Address& address );
	DataTransferSession( const DataTransferSession& ) = delete;
	virtual ~DataTransferSession();
	virtual int32_t handle_event( uint32_t flags );
	int exec_command( int32_t commandType, std::string params );
	void set_filepath( std::string filePath ) { fFilePath = filePath; }
	std::string filePath() { return fFilePath; }
	void set_accept_time( int64_t acceptTime ) { fAcceptTime = acceptTime; }
	int64_t accept_time() { return fAcceptTime; }
	void push( Packet* ptrPacket )
	{
		fPriSendQueue.push( ptrPacket );
	}
	Packet* front()
	{
		return fPriSendQueue.front();
	}
	void pop()
	{
		fPriSendQueue.pop();
	}
	bool empty()
	{
		return fPriSendQueue.empty();
	}
private:
	PriorityQueue<Packet*> fPriSendQueue;
	FTPSession *fFTPSession;
	int64_t fAcceptTime;
	std::mutex fReadMx;
	std::mutex fWriteMx;
	uint32_t	fstate;
	std::string fFilePath;
	std::string fCurrentDir;
	bool fReadFinished;
};

