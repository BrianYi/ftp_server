#include "Dispatcher.h"
//#include "Task.h"
#include "TaskThread.h"
#include "Log.h"

int Dispatcher::sFdEpoll = epoll_create( 64 );// size is ignored
int Dispatcher::sMaxevents = 64;
uint32_t Dispatcher::sThreadPicker = 0;
std::unordered_map<int, EventHandler*> Dispatcher::sHandlerTable;
std::mutex Dispatcher::mx;

Dispatcher::Dispatcher( )
{
}

void* Dispatcher::entry( )
{
	handle_events( );
	return nullptr;
}

void Dispatcher::handle_events( )
{
	struct epoll_event ev[sMaxevents];
	for ( ;; )
	{
		int nfds = epoll_wait( sFdEpoll, ev, sMaxevents, -1 );
		if ( nfds == -1 )
		{
			RTMP_Log( RTMP_LOGERROR,  "epoll_wait error %d", errno );
			continue;
		}
		for ( int i = 0; i < nfds; ++i )
		{
			std::unique_lock<std::mutex> lock( mx );
			auto it = sHandlerTable.find( ev[ i ].data.fd );
			EventHandler *handler = it->second;
#if DEBUG_EPOLL
			RTMP_LogAndPrintf( RTMP_LOGDEBUG, "fd %d triggered event=%s, handler events=%s",
								handler->fSocket,
								events_str( ev[ i ].events ).c_str( ),
								events_str( handler->events( ) ).c_str( ) );
#endif
			// add one task to task thread
			Task *task = new Task( handler, ev[ i ].events );
			this->push_to_thread( task );
		}
	}
}

void Dispatcher::register_handler( int fd, EventHandler* handler )
{
	struct epoll_event ev;
	ev.data.fd = handler->fSocket;
	ev.events = handler->events( );
	
	std::unique_lock<std::mutex> lock( mx );

	// judge if event exist
	auto it = sHandlerTable.find( fd );
	if ( it == sHandlerTable.end() )
	{
		/*
		 * event doesn't exist, register new one
		 */
		sHandlerTable.insert( std::make_pair( fd, handler ) );
#if DEBUG_EPOLL
		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "insert fd=%d", fd );
		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "EPOLL_CTL_ADD fd %d request event=%s, total-fd-num=%d, time=%llu",
						   fd,
						   events_str( handler->events() ).c_str(),
						   sHandlerTable.size( ),
						   get_timestamp_ms());
#endif
		if ( epoll_ctl( sFdEpoll, EPOLL_CTL_ADD, fd, &ev ) < 0 )
		{
			RTMP_LogAndPrintf( RTMP_LOGERROR, "epoll set insertion error: fd=%d, errno=%d",
					  fd,
					  errno );
			return;
		}
	}
	else
	{
		/*
		 * event exist
		 */
#if DEBUG_EPOLL
		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "EPOLL_CTL_MOD fd %d request event=%s, total-fd-num=%d, time=%llu",
						   fd,
						   events_str( handler->events( ) ).c_str( ),
						   sHandlerTable.size( ),
						   get_timestamp_ms());
#endif
		if ( epoll_ctl( sFdEpoll, EPOLL_CTL_MOD, fd, &ev ) < 0 )
		{
			RTMP_LogAndPrintf( RTMP_LOGERROR, "epoll set modify error: fd=%d, errno=%d",
					  fd,
					  errno );
			return;
		}
	}

}

void Dispatcher::remove_handler( int fd )
{
	std::unique_lock<std::mutex> lock( mx );

	auto it = sHandlerTable.find( fd );
	if ( it == sHandlerTable.end( ) )
	{
		RTMP_LogAndPrintf( RTMP_LOGERROR, "remove_handler error, don't have fd=%d", fd );
		return;
	}

	
	if ( epoll_ctl( sFdEpoll, EPOLL_CTL_DEL, fd, nullptr ) < 0 )
	{
		RTMP_LogAndPrintf( RTMP_LOGERROR, "epoll set remove error: fd=%d, errno=%d",
				fd,
				errno);
		return;
	}

#if DEBUG_EPOLL
	RTMP_LogAndPrintf( RTMP_LOGDEBUG, "EPOLL_CTL_DEL: delete fd=%d, event=%s, totoal fd=%d, time=%llu",
					   it->first,
					   events_str( it->second->events( ) ).c_str( ),
					   sHandlerTable.size( ),
					   get_timestamp_ms());
#endif

	// don't delete here, delete in TaskThread
	EventHandler *handler = it->second;
	sHandlerTable.erase( it );

	// delete in TaskThread
	Task *task = new Task( handler, Task::killEvent );
	push_to_thread( task );
	//close( fd ); // close should be in destructor
}

bool Dispatcher::exist_handler( int fd )
{
	return sHandlerTable.count( fd );
}
