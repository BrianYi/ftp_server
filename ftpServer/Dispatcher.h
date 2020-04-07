#pragma once
#include "Thread.h"
#include <sys/epoll.h>
#include "EventHandler.h"
#include <unordered_map>
#include <mutex>
class Dispatcher :
	public Thread
{ 
public:
	Dispatcher( );
	virtual ~Dispatcher( ) { };
	void* entry( );
	void handle_events( );
	static void register_handler( int fd, EventHandler* handler);
	static void remove_handler( int fd );
private:
	static int sFdEpoll;
	static int sMaxevents;
	static size_t sThreadPicker;
	static std::mutex mx;
	static std::unordered_multimap<int, EventHandler*> sHandlerTable;
};

