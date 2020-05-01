#pragma once
#include "TcpSocket.h"

class DataTransferSession;
class FTPSession;

class DataTransferListener: public TcpSocket
{
public:
	DataTransferListener( FTPSession *ftpSession, std::string ftpHome );
	virtual ~DataTransferListener();
	//virtual int32_t handle_event( uint32_t flags );
	DataTransferSession* accept( IOType inIOType );
	void handle_accept();
private:
	FTPSession *fFTPSession;
	std::mutex fReadMx;
	std::string fFtpHome;
	bool fAccepted;
};

