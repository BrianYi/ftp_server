/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "common.h"
#include "Log.h"

// packet info
#define KEEP_TRACK_PACKET			0
// packet hex info
#define KEEP_TRACK_PACKET_HEX		0
// only debug mode use, to caculate time wasting
#define TIME_CACULATE				0
#define DEBUG_EPOLL					0
#define DEBUG_RTMPSESSION_QUEUE		0
#define DEBUG_RTMPSESSION_READER	0
#define DEBUG_RTMPSESSION_WRITER	0
#define DEBUG_EVENT					0


#if KEEP_TRACK_PACKET
#define KEEP_TRACK_PACKET_SND	1
#define KEEP_TRACK_PACKET_RCV	1
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
	OnlineStreams,
	BuildConnect,
	TypeNum
};
