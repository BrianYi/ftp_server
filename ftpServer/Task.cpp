#include "Task.h"
#include "EventHandler.h"


Task::Task( EventHandler *handler, uint32_t flags )
{
	fHandler = handler;
	fFlags = flags;

// 	fTimestamp = 0;
// 	fSeq = 0;
}

void Task::run( )
{
	if (!(fFlags & killEvent))
		fHandler->handle_event(fFlags );
}
