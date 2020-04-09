#include "TaskThread.h"

std::vector<TaskThread*> TaskThreadPool::sTaskThreadArry;

void* TaskThread::entry( )
{
	for ( ;; )
	{
		if ( fTaskQueue.empty( ) )
		{
			sleep( 10 );
			continue;
		}

		std::unique_lock<std::mutex> lock( mx );
		Task *task = fTaskQueue.front( );;
		fTaskQueue.pop( );
		if ( task->get_flags() & Task::killEvent )
		{
			EventHandler* handler = task->get_handler( );
#ifdef _DEBUG
			RTMP_LogAndPrintf(RTMP_LOGDEBUG, "threadID=%llu, delete handler=%x, fTaskQueue.size=%d\n",
					this->get_handle( ),
					handler,
					this->fTaskQueue.size( ));
#endif // _DEBUG
			delete handler;
		}
		lock.unlock( );

		// must lock in implementation(read lock and write lock, or entire lock)
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
