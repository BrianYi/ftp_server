/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#include "Thread.h"

Thread::Thread( )
{
	fThreadId = -1;
}

int Thread::start( )
{
	int err = pthread_create( &fThreadId, nullptr, _entry, ( void * ) this );
	return err;
}

uint64_t Thread::handle( )
{
	return fThreadId;
}

void* Thread::_entry( void* arg )
{
	Thread* ptrThread = ( Thread * ) arg;
	ptrThread->entry( );
	return nullptr;
}
