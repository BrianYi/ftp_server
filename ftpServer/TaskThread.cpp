#include "TaskThread.h"

std::vector<TaskThread*> TaskThreadPool::sTaskThreadArry;
std::shared_mutex TaskThreadPool::sMutexRW;
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
				RTMP_LogAndPrintf( RTMP_LOGDEBUG, "Kill Event, event ref=%d, delete handler=%x, fd=%d",
								   h->refcount( ), h, h->fSocket );
				delete h;
				delete task;
			}
			else
				fTaskQueue.push( task );
			continue;
		}
		lock.unlock( );

		if (task->flags() & EPOLLIN ) // write lock
		{
			TaskThreadPool::sMutexRW.lock( );
			TaskThreadPool::sWriterNum++;
			RTMP_LogAndPrintf( RTMP_LOGDEBUG, "current thread=%llu fTaskQueue.size=%u, writers=%d",
							   this->handle( ), fTaskQueue.size( ), TaskThreadPool::sWriterNum.load() );
			task->run( );
			TaskThreadPool::sWriterNum--;
			TaskThreadPool::sMutexRW.unlock( );
		}
		else // read lock
		{
			TaskThreadPool::sMutexRW.lock_shared( );
			TaskThreadPool::sReaderNum++;
			RTMP_LogAndPrintf( RTMP_LOGDEBUG, "current thread=%llu fTaskQueue.size=%u, readers=%d",
							   this->handle( ), fTaskQueue.size( ), TaskThreadPool::sReaderNum.load() ); 
			task->run( );
			TaskThreadPool::sReaderNum--;
			TaskThreadPool::sMutexRW.unlock_shared( );
		}
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
