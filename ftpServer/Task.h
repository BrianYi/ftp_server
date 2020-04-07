#pragma once
#include "common.h"

class Task
{ 
public:
	enum
	{
		kill = 0x00000001,
		alive = 0xfffffffe
	};
	Task(int flags);
	~Task( );
	virtual int32_t run() = 0;
	void set_events( uint32_t events );
	void set_flags( uint32_t flags );
	uint32_t get_events();
	uint32_t get_flags( );
	static uint32_t get_task_num( );
protected:
	uint32_t fEvents;
	static uint32_t sTotalTaskNum;
	int32_t fFlags;
};

