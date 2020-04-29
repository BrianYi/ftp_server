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
		if ( fTaskPriQueue.empty( ) )
		{
			::usleep( 100 );
			continue;
		}

		/*
		 * get out one task
		 */
		Task *task = fTaskPriQueue.front( );
		fTaskPriQueue.pop( );

		/*
		 * judge if event should be killed?
		 */
		if ( task->flags() & Task::killEvent )
		{
			//std::unique_lock<std::mutex> lock( mx );
			EventHandler *h = task->handler();
			if ( h->refcount( ) == 1 )
			{
#if DEBUG_TaskThread
				printf( "Kill Event, event ref=%d, delete handler=%x, fd=%d\n",
								   h->refcount( ), h, h->fSocket );
#endif
				delete h;
				delete task;
			}
			else
				fTaskPriQueue.push( task );
			continue;
		}

#if DEBUG_TaskThread
		printf( "threadId=%d, taskQueue.size=%d, throw=%lld\n",
			this, fTaskPriQueue.size(), fThrowOutPacketNum.load() );
#endif

		/*
		 * has to wait a few time to run this task
		 */
		uint64_t curTimestamp = get_timestamp_ms();
		int64_t waitForTime = task->timestamp() - curTimestamp;
		if ( waitForTime > 0 )
		{
			fTaskPriQueue.push( task );
			::usleep( 100 );
			continue;
		}
		else
		{
			if ( -waitForTime > TASK_TIMEOUT )
			{
				fThrowOutPacketNum++;
				delete task;
				continue;
			}
		}

		/*
		 * run this task, return value means:
		 * < 0 : finished, delete this task
		 * >=0 : wait a few time, then re-run this task
		 */
		waitForTime = task->run();
		if ( waitForTime < 0 )
			delete task;
		else
		{
			uint64_t nextTime = get_timestamp_ms() + waitForTime;
			task->set_timestamp( nextTime );
			fTaskPriQueue.push( task );
		}
	}
	return nullptr;
}

void TaskThread::push( Task* task )
{
	fTaskPriQueue.push( task );
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
