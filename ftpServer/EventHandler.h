#pragma once
#include "common.h"

class Task;

class EventHandler
{ 
public:
	EventHandler( struct epoll_event ev, Task* task );
	~EventHandler( );
	struct epoll_event get_event( );
	void set_event( struct epoll_event event );
	Task *get_task( );
protected:
	struct epoll_event fEvent;
	Task *fTask;
};