/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "PlatformHeader.h"

class EventHandler;

class Task
{ 
public:
	enum
	{
		killEvent = 0x80000000,
	};
	Task( EventHandler *handler, uint32_t flags );
	~Task( );
	void run( );
	uint32_t flags( );
	void set_flags( uint32_t flags );
	EventHandler *handler( );
	uint64_t timestamp( ) { return fTimestamp; }
protected:
	EventHandler *fHandler;
	uint32_t fFlags;
	uint64_t fTimestamp;
};

inline uint32_t Task::flags( )
{
	return fFlags;
}

inline void Task::set_flags( uint32_t flags )
{
	fFlags = flags;
}

inline EventHandler * Task::handler( )
{
	return fHandler;
}