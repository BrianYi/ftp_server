#include "EventHandler.h"
#include "Task.h"

EventHandler::EventHandler( struct epoll_event ev, Task* task )
{
	fEvent = ev;
	fTask = task;
}

EventHandler::~EventHandler( )
{
// 	if (fTask )
// 		delete fTask;
}

struct epoll_event EventHandler::get_event( )
{
	return fEvent;
}

void EventHandler::set_event( struct epoll_event event )
{
	fEvent = event;
}

Task * EventHandler::get_task( )
{
	return fTask;
}
