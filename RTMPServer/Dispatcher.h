#pragma once
#include "Thread.h"
#include "EventHandler.h"
#include "TaskThread.h"
#include "Task.h"
#include <sys/epoll.h>
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
	static bool exist_handler( int fd );
private:
	static void push_to_thread( Task *task );
	static int sFdEpoll;
	static int sMaxevents;
	static uint32_t sThreadPicker;
	static std::mutex mx;
	static std::unordered_map<int, EventHandler*> sHandlerTable; // fd-handler
};

inline void Dispatcher::push_to_thread( Task *task )
{
	sThreadPicker++;
	sThreadPicker %= TaskThreadPool::get_num_threads( );
	TaskThread *taskThread = TaskThreadPool::get_thread( sThreadPicker );
	taskThread->push( task );
}