/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "Thread.h"
#include "Task.h"
#include "Queue.h"

class TaskThread :
	public Thread
{ 
public:
	TaskThread( ) { fThrowOutPacketNum  = 0;};
	~TaskThread( ) { };
	void* entry( );
	void push( Task* task )
	{
		fTaskPriQueue.push( task );
	}
	size_t size() { return fTaskPriQueue.size(); }
private:
	PriorityQueue<Task*> fTaskPriQueue;
	std::atomic<uint64_t> fThrowOutPacketNum;
	//std::mutex mx;
};

class TaskThreadPool
{
public:
	static uint32_t add_thread( uint32_t numThread );
	static TaskThread* get_thread( uint32_t index );
	static uint32_t get_num_threads( );
	static TaskThread* pick_thread();
	static std::atomic<int32_t> sReaderNum,sWriterNum;
private:
	static std::vector<TaskThread*> sTaskThreadArry;
};