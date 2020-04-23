/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "RtmpSession.h"
#include <unordered_map>
#include <algorithm>

class RtmpSessionTable
{ 
public:
	//typedef std::vector<RtmpSession*> RtmpSessionArryType;
	typedef std::unordered_map<std::string, RtmpSession*> RtmpSessionUnorderedMapType;
	typedef std::unordered_multimap<std::string, RtmpSession*> RtmpSessionUnorderedMultimapType;
public:
	RtmpSessionTable( );
	~RtmpSessionTable( );
	static void insert( RtmpSession *rtmpSession )
	{
		std::unique_lock<std::mutex> lock( RtmpSessionTable::mx );
		// insert into session table
		if ( sRtmpSessionTable.count( rtmpSession->app() ) )
		{
			RTMP_Log( RTMP_LOGERROR, "insert duplicated, rtmpSession=%x, [%s:%d]",
					  rtmpSession, __FUNCTION__, __LINE__ );
			return;
		}
		else
			sRtmpSessionTable.insert( std::make_pair( rtmpSession->app(), rtmpSession ) );
	}
	static void insert( const std::string& app, RtmpSession *rtmpSession )
	{
		std::unique_lock<std::mutex> lock( RtmpSessionTable::mx );
		// insert into pair table
		sRtmpPairSessionTable.insert( std::make_pair(app, rtmpSession ));
	}
	static void erase( RtmpSession *rtmpSession )
	{
		std::unique_lock<std::mutex> lock( RtmpSessionTable::mx );
		
		// remove from session table
		if ( !sRtmpSessionTable.count( rtmpSession->app() ) )
		{
			RTMP_Log( RTMP_LOGERROR, "erase failed, rtmpSession=%x, [%s:%d]",
					  rtmpSession, __FUNCTION__, __LINE__ );
			return;
		}
		else
			sRtmpSessionTable.erase( rtmpSession->app() );
		
		// remove from pairs table
		auto it = sRtmpPairSessionTable.begin();
		while ( it != sRtmpPairSessionTable.end() )
		{
			if ( it->first == rtmpSession->app() ||
				 it->second->app() == rtmpSession->app() )
				it = sRtmpPairSessionTable.erase( it );
			else
				++it;
		}

//		auto range = sRtmpPairSessionTable.equal_range( rtmpSession->app() );
// 		auto it = range.first;
// 		while(it != range.second)
// 		{
// 			auto range2 = sRtmpPairSessionTable.equal_range( it->second->app() );
// 			auto it2 = range2.first;
// 			while ( it2 != range2.second )
// 			{
// 				if ( it2->second->app() == rtmpSession->app() )
// 					it2 = sRtmpPairSessionTable.erase( it2 );
// 				else
// 					++it2;
// 			}
// 			++it;
// 		}
	}
	static bool is_exist_app( const std::string& app ) { return sRtmpSessionTable.count( app ); }
// 	static std::pair<RtmpSessionUnorderedMultimapType::iterator, RtmpSessionUnorderedMultimapType::iterator> 
// 		equal_range_pullers( const std::string& app ) 
// 	{ 
// 		// need global lock
// 		return sRtmpPullerSessionTable.equal_range( app ); 
// 	}
	static int32_t timebase( const std::string& app )
	{
		assert( sRtmpSessionTable.count( app ) );
		RtmpSession *rtmpSession = sRtmpSessionTable[ app ];
		return rtmpSession->timebase( );
	}
	static std::string app_names()
	{
		std::unique_lock<std::mutex> lock( RtmpSessionTable::mx );
		std::string appnames;
		for ( auto it = sRtmpSessionTable.begin();
			  it != sRtmpSessionTable.end();
			  ++it )
		{
			RtmpSession *rtmpSession = it->second;
			appnames += rtmpSession->app() + "\n";
		}
		return appnames;
	}
	static void broadcast( const Packet& packet )
	{
		std::unique_lock<std::mutex> lock( RtmpSessionTable::mx );
		for (auto it = sRtmpSessionTable.begin();
			  it != sRtmpSessionTable.end();
			  ++it)
		{
			RtmpSession *rtmpSession = it->second;
			if ( rtmpSession->app() == packet.app() ) continue;

			rtmpSession->queue_push( PacketUtils::new_packet(packet) );
			rtmpSession->request_event( EPOLLOUT );
		}
	}
	static void broadcast( const std::string& app, const Packet& packet )
	{
		std::unique_lock<std::mutex> lock( RtmpSessionTable::mx );
		auto range = sRtmpPairSessionTable.equal_range( app );
		for ( auto it = range.first; it != range.second; ++it )
		{
			RtmpSession *rtmpSession = it->second;
			rtmpSession->queue_push( PacketUtils::new_packet( packet ) );
			// request write event
			rtmpSession->request_event( EPOLLOUT );
		}
	}
	static int32_t session_count() { return sRtmpSessionTable.size(); }
	static int32_t pair_count( ) { return sRtmpPairSessionTable.size( ); }
	static int32_t pusher_count() { return sRtmpPairSessionTable.size() / 2; }
	static int32_t puller_count( const std::string& app ) { return sRtmpPairSessionTable.count( app ); }
	static RtmpSession* find(const std::string& app) 
	{ 
		if ( !sRtmpSessionTable.count( app ) )
			return nullptr;
		return sRtmpSessionTable[ app ];
	}
// 	static RtmpSession* find_pusher( const std::string& app ) 
// 	{ 
// 		auto it = sRtmpPusherSessionTable.find( app );
// 		return (it != sRtmpPusherSessionTable.end( ) ? it->second : nullptr);
// 	}
protected:
	//static RtmpSessionArryType sRtmpSessionArry;	// store all session
	static RtmpSessionUnorderedMapType sRtmpSessionTable;		// store pusher app-session
	static RtmpSessionUnorderedMultimapType sRtmpPairSessionTable;	// store puller app-session
	static std::mutex mx;
};
