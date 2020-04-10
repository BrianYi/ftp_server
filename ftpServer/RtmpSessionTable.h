#pragma once
#include "RtmpSession.h"
#include <unordered_map>
#include <algorithm>

class RtmpSessionTable
{ 
public:
	typedef std::vector<RtmpSession*> RtmpSessionArryType;
	typedef std::unordered_map<std::string, RtmpSession*> RtmpSessionUnorderedMapType;
	typedef std::unordered_multimap<std::string, RtmpSession*> RtmpSessionUnorderedMultimapType;
public:
	RtmpSessionTable( );
	~RtmpSessionTable( );
	static void insert( RtmpSession *rtmpSession )
	{
		std::unique_lock<std::mutex> lock( RtmpSessionTable::mx );
		auto cmpArry = [ & ] ( RtmpSession *r ) { return r == rtmpSession; };
		if ( std::find_if( sRtmpSessionArry.begin( ), sRtmpSessionArry.end( ), cmpArry ) != sRtmpSessionArry.end( ) )
		{
			RTMP_Log( RTMP_LOGDEBUG, "insert duplicated, rtmpSession=%x, [%s:%d]",
					  rtmpSession, __FUNCTION__, __LINE__ );
			return;
		}
		sRtmpSessionArry.push_back( rtmpSession );
		if ( rtmpSession->type( ) == RtmpSession::typePusher )
			sRtmpPusherSessionTable.insert( std::make_pair( rtmpSession->app( ), rtmpSession ) );
		else if ( rtmpSession->type( ) == RtmpSession::typePuller )
			sRtmpPullerSessionTable.insert( std::make_pair( rtmpSession->app( ), rtmpSession ) );
	}
	static void erase( RtmpSession *rtmpSession )
	{
		std::unique_lock<std::mutex> lock( RtmpSessionTable::mx );
		auto cmpArry = [& ] ( RtmpSession *r ) { return r == rtmpSession; };
		auto it = std::find_if( sRtmpSessionArry.begin( ), sRtmpSessionArry.end( ), cmpArry );
		if ( it == sRtmpSessionArry.end( ) )
		{
			RTMP_Log( RTMP_LOGDEBUG, "erase failed, rtmpSession=%x, [%s:%d]",
					  rtmpSession, __FUNCTION__, __LINE__ );
			return;
		}
		sRtmpSessionArry.erase( it );
		if ( rtmpSession->type( ) == RtmpSession::typePusher )
		{
			auto it = sRtmpPusherSessionTable.find( rtmpSession->app( ) );
			if ( it->second == rtmpSession )
			{
				sRtmpPusherSessionTable.erase( it );
				RTMP_Log( RTMP_LOGDEBUG, "erase pusher fd %d",
						  rtmpSession->fSocket );
			}
		}
		else if ( rtmpSession->type( ) == RtmpSession::typePuller )
		{
			auto range = sRtmpPullerSessionTable.equal_range( rtmpSession->app( ) );
			auto it = range.first; 
			while ( it != range.second )
			{
				if (it->second == rtmpSession )
				{
					sRtmpPullerSessionTable.erase( it );
					RTMP_Log( RTMP_LOGDEBUG, "erase puller fd %d",
							  rtmpSession->fSocket );
					break;
				}
				++it;
			}
		}
	}
	static bool is_exist_app( const std::string& app ) { return sRtmpPusherSessionTable.count( app ); }
// 	static std::pair<RtmpSessionUnorderedMultimapType::iterator, RtmpSessionUnorderedMultimapType::iterator> 
// 		equal_range_pullers( const std::string& app ) 
// 	{ 
// 		// need global lock
// 		return sRtmpPullerSessionTable.equal_range( app ); 
// 	}
	static int32_t timebase( const std::string& app )
	{
		auto it = sRtmpPusherSessionTable.find( app );
		return it->second->timebase( );
	}
	static void broadcast( const std::string& app, Packet& packet )
	{
		std::unique_lock<std::mutex> lock( RtmpSessionTable::mx );
		auto range = sRtmpPullerSessionTable.equal_range( app );
		for ( auto it = range.first; it != range.second; ++it )
		{
			RtmpSession *rtmpSession = it->second;

			if (packet.type() == Push )
			{
				rtmpSession->queue_push( PacketUtils::new_pull_packet(
					packet.size( ), packet.MP( ), packet.seq( ), packet.timestamp( ),
					packet.app( ), packet.body( ) ) );
			}
			else if ( packet.type( ) == Fin )
			{
				rtmpSession->queue_push( PacketUtils::new_fin_packet(
					packet.timestamp( ), packet.app( ) ) );
			}
			else
			{
				RTMP_LogAndPrintf( RTMP_LOGDEBUG, "Unknown broadcast packet, [%s:%d]", 
								   __FUNCTION__, __LINE__ );
			}

			// request write event
			rtmpSession->request_event( EPOLLOUT );
		}
	}
// 	static RtmpSession* find_pusher( const std::string& app ) 
// 	{ 
// 		auto it = sRtmpPusherSessionTable.find( app );
// 		return (it != sRtmpPusherSessionTable.end( ) ? it->second : nullptr);
// 	}
protected:
	static RtmpSessionArryType sRtmpSessionArry;	// store all session
	static RtmpSessionUnorderedMapType sRtmpPusherSessionTable;		// store pusher app-session
	static RtmpSessionUnorderedMultimapType sRtmpPullerSessionTable;	// store puller app-session
	static std::mutex mx;
};
