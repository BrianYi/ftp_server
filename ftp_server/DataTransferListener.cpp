#include "DataTransferListener.h"
#include "DataTransferSession.h"
#include "FTPSession.h"
#include "Log.h"

DataTransferListener::DataTransferListener( FTPSession *ftpSession, std::string ftpHome )
{
	fFTPSession = ftpSession;
	fFtpHome = ftpHome;
}

DataTransferListener::~DataTransferListener()
{

}

DataTransferSession* DataTransferListener::accept( IOType inIOType )
{
	if ( this->fIOType != inIOType )
		this->setIOType( inIOType );

	socklen_t len = sizeof( struct sockaddr );
	Address address;
	int socketID = ::accept( this->fSocket, (struct sockaddr*)&address, &len );

	if ( socketID == -1 ) return nullptr;

	DataTransferSession *dataTransSession = new DataTransferSession( socketID, fFTPSession, fFtpHome );
	return dataTransSession;
}

void DataTransferListener::handle_accept()
{
	// lock reading
	std::unique_lock<std::mutex> lockRead( fReadMx );

	if ( !lockRead.owns_lock() )
		return ;
	else
	{
		// this shot is triggered
		this->fEvents &= ~EPOLLIN;

		DataTransferSession* dataTransSession = this->accept( Blocking );
		if ( dataTransSession == nullptr )
		{
			printf( "errno=%d\n", errno );
			assert( false );
			//this->request_event( EPOLLIN );
			return ;
		}

		fFTPSession->push_data_transfer_session( dataTransSession );
		dataTransSession->set_accept_time( get_timestamp_ms() );
		dataTransSession->request_event( EPOLLIN );
	}
}
