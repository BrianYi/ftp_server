#include <cstdio>
#include "Dispatcher.h"
#include "TaskThread.h"
#include "RtmpListenerSocket.h"


int main( int argc, char* argv[ ] )
{
	/*
	 * Log thread start
	 */
	FILE* dumpfile = nullptr;
	if ( argv[ 1 ] )
		dumpfile = fopen( argv[ 1 ], "a+" );
	else
		dumpfile = fopen( "rtmp_server.dump", "a+" );
	RTMP_LogSetOutput( dumpfile );
	RTMP_LogSetLevel( RTMP_LOGALL );
	RTMP_LogThreadStart( );
#if WIN32
	SYSTEMTIME tm;
	GetSystemTime( &tm );
#elif __linux__
	time_t t = time( NULL );
	struct tm tm = *localtime( &t );
#define wYear tm_year+1900
#define wMonth tm_mon+1
#define wDay tm_mday
#define wHour tm_hour
#define wMinute tm_min
#define wSecond tm_sec
#define wMilliseconds 0
#endif
	RTMP_Log( RTMP_LOGDEBUG, "==============================" );
	RTMP_Log( RTMP_LOGDEBUG, "log file:\trtmp_server.dump" );
	RTMP_Log( RTMP_LOGDEBUG, "log timestamp:\t%lld", get_timestamp_ms( ) );
	RTMP_Log( RTMP_LOGDEBUG, "log date:\t%d-%d-%d %d:%d:%d",
			  tm.wYear,
			  tm.wMonth,
			  tm.wDay,
			  tm.wHour, tm.wMinute, tm.wSecond );
	RTMP_Log( RTMP_LOGDEBUG, "==============================" );

	/*
	 * Dispatcher thread start
	 */
	Dispatcher* dispatcher = new Dispatcher;
	dispatcher->start( );

	/*
	 * Task threads start
	 */
	uint32_t num_of_processors = sysconf( _SC_NPROCESSORS_ONLN );
	TaskThreadPool::add_thread( num_of_processors );
	
	RtmpListenerSocket *rtmpListenerSocket = new RtmpListenerSocket;
	rtmpListenerSocket->listen( SERVER_PORT_TCP, 64 );
	rtmpListenerSocket->request_event( EPOLLIN | EPOLLET );

	for ( ;; )
	{
		sleep( 1000 );
	}

	delete rtmpListenerSocket;
	delete dispatcher;
    return 0;
}