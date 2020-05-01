/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#include <cstdio>
#include "ServerHeader.h"
#include "Dispatcher.h"
#include "TaskThread.h"
#include "FTPListener.h"
#include "Log.h"
#include <sys/stat.h>
#include <dirent.h>

int main( int argc, char* argv[ ] )
{
	/*
	 * Log thread start
	 */
	FILE* dumpfile = nullptr;
	if ( argv[ 1 ] )
		dumpfile = fopen( argv[ 1 ], "a+" );
	else
		dumpfile = fopen( "ftp_server.dump", "a+" );
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
	RTMP_Log( RTMP_LOGDEBUG, "log file:\tftp_server.dump" );
	RTMP_Log( RTMP_LOGDEBUG, "log timestamp:\t%lld", (long long int)get_timestamp_ms( ) );
	RTMP_Log( RTMP_LOGDEBUG, "log date:\t%d-%d-%d %d:%d:%d",
			  tm.wYear,
			  tm.wMonth,
			  tm.wDay,
			  tm.wHour, tm.wMinute, tm.wSecond );
	RTMP_Log( RTMP_LOGDEBUG, "==============================" );

	/*
	 * Create ftp shared dir
	 */
	std::string homedir = getenv( "HOME" );
	std::string ftpHome = homedir + "/ftp_shared";
	DIR *dir = opendir( ftpHome.c_str() );
	if ( dir )
		closedir( dir );
	else
	{
		if ( mkdir( ftpHome.c_str(), S_IRWXU | S_IRWXG | S_IRWXO ) != 0 )
		{
			printf( "Cannot create default ftp shared path %s: %d\n", ftpHome.c_str(), errno );
			exit( EXIT_FAILURE );
		}
	}

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

	/*
	 * Register listener
	 */
	FTPListener *ftpListener = new FTPListener(ftpHome);
	ftpListener->listen( SERVER_COMMAND_PORT, 64 );
	ftpListener->request_event( EPOLLIN );

	for ( ;; )
	{
		sleep( 5 );
	}

	delete ftpListener;
	delete dispatcher;
	exit( EXIT_SUCCESS );
}
