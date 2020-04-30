/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "PlatformHeader.h"
#include "Thread.h"
#include "EventHandler.h"
#include "TaskThread.h"
#include "Task.h"
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
	static void push_to_thread( Task *task );
private:
	static int sFdEpoll;
	static int sMaxevents;
	//static uint32_t sThreadPicker;
	static std::mutex sHandlerTableMx;
	static std::mutex sThreadPickerMx;
	static std::unordered_map<int, EventHandler*> sHandlerTable; // fd-handler
};

inline void Dispatcher::push_to_thread( Task *task )
{
	std::unique_lock<std::mutex> locker( sThreadPickerMx );
	//sThreadPicker++;
	//sThreadPicker %= TaskThreadPool::get_num_threads( );
	TaskThread *taskThread = TaskThreadPool::pick_thread();
	taskThread->push( task );
}