#pragma once
#include "TcpSocket.h"

class FTPSession;

class FTPListenerSocket :
	public TcpSocket
{
public:
	FTPListenerSocket(std::string ftpHome);
	virtual ~FTPListenerSocket();
	virtual int32_t handle_event( uint32_t flags );
	FTPSession* accept( IOType inIOType = Blocking );
private:
	std::mutex fReadMx;
	std::string fFtpHome;
};

