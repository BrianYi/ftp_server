#pragma once
#include "common.h"
#include "Log.h"

// packet info
#define KEEP_TRACK_PACKET		1
// packet hex info
#define KEEP_TRACK_PACKET_HEX	0
// only debug mode use, to caculate time wasting
#define TIME_CACULATE			0

#if KEEP_TRACK_PACKET
#define KEEP_TRACK_PACKET_SND
#define KEEP_TRACK_PACKET_RCV
#endif

#if KEEP_TRACK_PACKET_HEX
#define KEEP_TRACK_PACKET_SND_HEX
#define KEEP_TRACK_PACKET_RCV_HEX
#endif

const uint32_t RECV_BUF_SIZE = 10 * 1024u;
const uint32_t SEND_BUF_SIZE = 10 * 1024u;

#define SERVER_IP	"0.0.0.0"
#define SERVER_PORT_TCP	5566


enum
{
	CreateStream,
	Play,
	Push,
	Pull,
	Ack,
	Fin,
	Err,
	TypeNum
};
