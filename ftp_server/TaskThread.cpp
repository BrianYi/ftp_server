/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#include "TaskThread.h"
#include "EventHandler.h"

std::vector<TaskThread*> TaskThreadPool::sTaskThreadArry;
std::atomic<int32_t> TaskThreadPool::sReaderNum, TaskThreadPool::sWriterNum;

void* TaskThread::entry( )
{
	for ( ;; )
	{
		if ( fTaskQueue.empty( ) )
		{
			::usleep( 100 );
			continue;
		}

		std::unique_lock<std::mutex> lock( mx );
		Task *task = fTaskQueue.front( );
		fTaskQueue.pop( );
		if ( task->flags() & Task::killEvent )
		{
			EventHandler *h = task->handler( );
			if ( h->refcount( ) == 1 )
			{
#if DEBUG_EVENT
				RTMP_LogAndPrintf( RTMP_LOGDEBUG, "Kill Event, event ref=%d, delete handler=%x, fd=%d",
								   h->refcount( ), h, h->fSocket );
#endif
				delete h;
				delete task;
			}
			else
				fTaskQueue.push( task );
			continue;
		}
		lock.unlock( );

		task->run( );

		delete task;
	}
	return nullptr;
}

void TaskThread::push( Task* task )
{
	std::unique_lock<std::mutex> lock( mx );
	fTaskQueue.push( task );
}

uint32_t TaskThreadPool::add_thread( uint32_t numThread )
{
	for ( uint32_t i = 0; i < numThread; ++i )
	{
		TaskThread* ptrTaskThread = new TaskThread( );
		sTaskThreadArry.push_back( ptrTaskThread );
		ptrTaskThread->start( );
	}
	return sTaskThreadArry.size( );
}

TaskThread* TaskThreadPool::get_thread( uint32_t index )
{
	if ( index < 0 || index >= sTaskThreadArry.size( ) )
		return nullptr;
	return sTaskThreadArry[ index ];
}

uint32_t TaskThreadPool::get_num_threads( )
{
	return sTaskThreadArry.size( );
}
