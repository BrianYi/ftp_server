/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "PlatformHeader.h"

class Thread
{ 
	uint64_t fThreadId;
public:
	Thread( );
	virtual ~Thread( ) { }
	int start( );
	virtual void* entry( ) = 0;
	uint64_t handle( );
private:
	static void* _entry( void* arg );
};

