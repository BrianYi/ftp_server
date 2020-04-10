#include "Task.h"
#include "EventHandler.h"


Task::Task( EventHandler *handler, uint32_t flags )
{
	fHandler = handler;
	fFlags = flags;
	handler->inc_refcount( );
	fTimestamp = get_timestamp_ms( );
// 	fTimestamp = 0;
// 	fSeq = 0;
}

Task::~Task( )
{
// #ifdef _DEBUG
// 	RTMP_LogAndPrintf( RTMP_LOGDEBUG, "event ref=%d, handler=%x, fd=%d",
// 					   fHandler->refcount( ), fHandler, fHandler->fSocket );
// #endif // _DEBUG

	fHandler->dec_refcount( );

// #ifdef _DEBUG
// 	if ( fHandler->refcount() == 0 && this->fFlags & killEvent )
// 	{
// 		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "Kill Event, event ref=%d, delete handler=%x, fd=%d",
// 						   fHandler->refcount(), fHandler, fHandler->fSocket );
// 		delete fHandler;
// 	}
// #endif // _DEBUG
}

void Task::run( )
{
	if (!(fFlags & killEvent))
		fHandler->handle_event(fFlags );
}
