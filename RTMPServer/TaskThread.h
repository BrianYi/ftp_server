#pragma once
#include "Thread.h"
#include "EventHandler.h"
#include "Task.h"
#include <queue>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <atomic>

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
	static std::shared_mutex sMutexRW;
	static std::atomic<int32_t> sReaderNum,sWriterNum;
private:
	static std::vector<TaskThread*> sTaskThreadArry;
};