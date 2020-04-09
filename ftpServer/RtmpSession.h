#pragma once
#include "TcpSocket.h"
#include "Packet.h"
#include <queue>

class cmp
{
public:
	bool operator()( Packet* p1, Packet* p2 )
	{
		if ( p1->timestamp( ) > p2->timestamp( ) )
			return true;
		else if ( p1->timestamp( ) == p2->timestamp( ) )
			return p1->seq( ) > p2->seq( );
		return false;
	}
};

class RtmpSession :
	public TcpSocket
{ 
public:
	enum
	{
		typeUnknown,
		typePusher,
		typePuller
	};
	enum
	{
		stateIdle,
		statePushing,
		statePulling
	};
	RtmpSession( );
	RtmpSession( int32_t fd, Address& address );
	RtmpSession( const RtmpSession& ) = delete;
	virtual ~RtmpSession( );
	virtual int32_t handle_event( uint32_t flags );
	void set_app( std::string app ) { fApp = app; }
	std::string app( ) { return fApp; }
	void set_accept_time( int64_t acceptTime ) { fAcceptTime = acceptTime; }
	int64_t accept_time( ) { return fAcceptTime; }
	void set_timebase( int32_t timebase ) { fTimebase = timebase; }
	int32_t timebase( ) { return fTimebase; }
	void set_type( int32_t type ) { fType = type; }
	int32_t type( ) { return fType; }
	void queue_push( Packet* ptrPkt )
	{
		std::unique_lock<std::mutex> lock( this->fQueueMx );
		fPacketQueue.push( ptrPkt );
	}
	Packet* queue_front( )
	{
		return fPacketQueue.top( );
	}
	void queue_pop( )
	{
		std::unique_lock<std::mutex> lock( this->fQueueMx );
		fPacketQueue.pop( );
	}
	bool queue_empty( )
	{
		return fPacketQueue.empty( );
	}
protected:
	void disconnect( );
	void recv_report( Packet* ptrPkt );
	void send_report( Packet* ptrPkt );
protected:
	std::string fApp;
	int64_t fAcceptTime;
// 	int64_t fLastSendTime;
// 	int64_t fLastRecvTime;
	int32_t fTimebase;
	int32_t fType;
	int32_t fState;
	std::priority_queue<Packet*, std::vector<Packet*>, cmp> fPacketQueue;
	std::mutex fQueueMx;
};
