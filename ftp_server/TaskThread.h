/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "Thread.h"
#include "Task.h"

class TaskThread :
	public Thread
{ 
public:
	TaskThread( ) { };
	~TaskThread( ) { };
	void* entry( );
	void push( Task* task );
private:
	std::queue<Task*> fTaskQueue;
	std::mutex mx;
};

class TaskThreadPool
{
public:
	static uint32_t add_thread( uint32_t numThread );
	static TaskThread* get_thread( uint32_t index );
	static uint32_t get_num_threads( );
	static std::atomic<int32_t> sReaderNum,sWriterNum;
private:
	static std::vector<TaskThread*> sTaskThreadArry;
};