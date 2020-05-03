/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "TcpSocket.h"
#include "Queue.h"
#include "Packet.h"
#include "FTPSession.h"

/*
 * DataTranferSession is only work for data transmission
 * every FTPSession can owns many data transmission
 * it means for one user(one FTPSession), it could transfer over 500 files
 * (would establish 500 data transmission) one time
 */
class DataTransferSession :
	public TcpSocket
{
public:
	DataTransferSession( FTPSession *ftpSession, std::string currentDir );
	DataTransferSession( int32_t fd, FTPSession *ftpSession, std::string currentDir );
	//DataTransferSession( FTPSession *ftpSession, std::string currentDir, int32_t fd );
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
private:
	PriorityQueue<Packet*> fSendPriQueue;
	FTPSession *fFTPSession;
	int64_t fAcceptTime;
	std::mutex fReadMx;
	std::mutex fWriteMx;
	uint32_t	fstate;
	std::string fFilePath;
	std::string fCurrentDir;
	int32_t fFileDesc;
	bool fRcvFinished;
	int32_t fType;
};