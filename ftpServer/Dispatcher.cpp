#include "Dispatcher.h"
#include "Task.h"
#include "TaskThread.h"

int Dispatcher::sFdEpoll = epoll_create( 64 );// size is ignored
int Dispatcher::sMaxevents = 64;
size_t Dispatcher::sThreadPicker = 0;
std::unordered_multimap<int, EventHandler*> Dispatcher::sHandlerTable;
std::mutex Dispatcher::mx;

Dispatcher::Dispatcher( )
{
}

void* Dispatcher::entry( )
{
	handle_events( );
	return nullptr;
}

void Dispatcher::handle_events( ) // demultiplexer
{
	struct epoll_event events[sMaxevents];
	for ( ;; )
	{
		int nfds = epoll_wait( sFdEpoll, events, sMaxevents, -1 );
		if ( nfds == -1 )
		{
			printf( "epoll_wait error %d\n", errno );
			continue;
		}
		for ( int i = 0; i < nfds; ++i )
		{
			std::unique_lock<std::mutex> lock( mx );
			auto range = sHandlerTable.equal_range( events[ i ].data.fd );
			for ( auto it = range.first; it != range.second; ++it )
			{
				EventHandler *handler = it->second;
				struct epoll_event ev = handler->get_event( );
				if ( ev.events & events[i].events )
				{
					sThreadPicker++;
					sThreadPicker %= TaskThreadPool::get_num_threads( );
					TaskThread *taskThread = TaskThreadPool::get_thread( sThreadPicker );
					Task* task = handler->get_task( );
					task->set_events( events[ i ].events );
					taskThread->push( task );
				}
			}
		}
	}
}

void Dispatcher::register_handler( int fd, EventHandler* handler )
{
	struct epoll_event ev = handler->get_event( );
	
	std::unique_lock<std::mutex> lock( mx );
	if ( epoll_ctl( sFdEpoll, EPOLL_CTL_ADD, fd, &ev ) < 0 )
	{
		printf( "epoll set insertion error: fd=%d, errno=%d\n", 
				fd,
				errno);
		return;
	}
	sHandlerTable.insert( std::make_pair( fd, handler ) );
#ifdef _DEBUG
	printf( "insert fd=%d, event=%x, totoal fd=%d\n",
			fd,
			handler,
			sHandlerTable.size( ) );
#endif
}

void Dispatcher::remove_handler( int fd )
{
	std::unique_lock<std::mutex> lock( mx );
	if ( epoll_ctl( sFdEpoll, EPOLL_CTL_DEL, fd, nullptr ) < 0 )
	{
		printf( "epoll set remove error: fd=%d, errno=%d\n", 
				fd,
				errno);
		return;
	}
	auto range = sHandlerTable.equal_range( fd );
	for ( auto it = range.first; it != range.second; ++it )
	{
#ifdef _DEBUG
		printf( "delete fd=%d, event=%x, totoal fd=%d\n", 
				it->first, 
				it->second, 
				sHandlerTable.size() );
#endif
		delete it->second;
	}
	sHandlerTable.erase( fd );
}
