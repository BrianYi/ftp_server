#pragma once
#include "TcpSocket.h"

class FTPSession;

class FTPListener :
	public TcpSocket
{
public:
	FTPListener(std::string ftpHome);
	virtual ~FTPListener();
	virtual int32_t handle_event( uint32_t flags );
	FTPSession* accept( IOType inIOType );
private:
	std::mutex fReadMx;
	std::string fFtpHome;
};

