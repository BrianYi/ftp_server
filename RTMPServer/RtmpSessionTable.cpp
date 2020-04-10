#include "RtmpSessionTable.h"

std::mutex RtmpSessionTable::mx;
/*
 * static variable declaration
 */
RtmpSessionTable::RtmpSessionArryType RtmpSessionTable::sRtmpSessionArry;	// store all session
RtmpSessionTable::RtmpSessionUnorderedMapType RtmpSessionTable::sRtmpPusherSessionTable;		// store pusher app-session
RtmpSessionTable::RtmpSessionUnorderedMultimapType RtmpSessionTable::sRtmpPullerSessionTable;	// store puller app-session

RtmpSessionTable::RtmpSessionTable( )
{

}

RtmpSessionTable::~RtmpSessionTable( )
{

}
