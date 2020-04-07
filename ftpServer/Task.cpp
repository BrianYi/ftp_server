#include "Task.h"

uint32_t Task::sTotalTaskNum = 0;

Task::Task()
{
	sTotalTaskNum++;
}

Task::~Task( )
{
	sTotalTaskNum--;
}

void Task::set_events( uint32_t events )
{
	fEvents = events;
}

void Task::set_flags( uint32_t flags )
{
	fFlags = flags;
}

uint32_t Task::get_events()
{
	return fEvents;
}

uint32_t Task::get_flags( )
{
	return fFlags;
}

uint32_t Task::get_task_num( )
{
	return sTotalTaskNum;
}

void Task::request_event( uint32_t events )
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = this->fSocket;
	EventHandler *handler = new EventHandler( ev, this );
	Dispatcher::register_handler( this->fSocket, handler );
}

