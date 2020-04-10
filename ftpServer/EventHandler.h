#pragma once
#include "ServerHeader.h"
#include "Socket.h"
#include <mutex>
#include <atomic>

class EventHandler: public Socket
{ 
public:
	EventHandler( int32_t inSocketType, int32_t inProtocol, IOType inIOType );
	EventHandler( int32_t fd, bool binded, int32_t inSocketType, int32_t inProtocol, IOType inIOType );
	virtual ~EventHandler( );
	virtual void request_event( uint32_t events );
	uint32_t events( ) { return fEvents; }
	void set_events( uint32_t events ) 
	{ 
		// need atomic
		fEvents = events; 
	}
	virtual int32_t handle_event(uint32_t flags ) { return 0; };
	virtual void kill_event( );
	void set_dead( ) { fIsDead = true; }
	bool dead( ) { return fIsDead; }
	int32_t refcount( ) { return fRefCount.load( ); }
	void inc_refcount( ) { fRefCount++; }
	void dec_refcount( ) { fRefCount--; }
protected:
	uint32_t fEvents;
	//std::mutex fReadMx;
	//std::mutex fWriteMx;
	bool fIsDead;
	std::atomic<int32_t> fRefCount;
};

/*
	 * debug function
	 */
std::string events_str( uint32_t events );