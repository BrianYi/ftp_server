/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "ServerHeader.h"

#define MAX_BODY_SIZE 1400
#define MAX_PACKET_SIZE (MAX_BODY_SIZE+sizeof (HEADER))

#define BODY_SIZE(MP,size,seq)	(MP?MAX_BODY_SIZE:size - seq)
#define BODY_SIZE_H(header)		BODY_SIZE(header.MP,header.size,header.seq)
#define PACK_SIZE(MP,size,seq)	(MP?MAX_PACKET_SIZE:(sizeof (HEADER)+BODY_SIZE(MP,size,seq)))
#define PACK_SIZE_H(header)		PACK_SIZE(header.MP,header.size,header.seq)
#define NUM_PACK(size)			((size + MAX_BODY_SIZE - 1) / MAX_BODY_SIZE)
#define LAST_PACK_SEQ(size)		((size / MAX_BODY_SIZE) * MAX_BODY_SIZE)
#define INVALID_TYPE(header)	(header.type < 0 || header.type >= TypeNum)
#define INVALID_SEQ(header)		(header.seq % MAX_BODY_SIZE)
#define INVALID_SIZE(header)	(PACK_SIZE_H(header)>MAX_PACKET_SIZE || PACK_SIZE_H(header)<sizeof HEADER)
#define INVALID_PACK(header)	(INVALID_TYPE(header)||INVALID_SEQ(header)||INVALID_SIZE(header))

 // 4+4+4+8+8+16=44
#pragma pack(1)
struct HEADER
{
	// total body size
	uint32_t size;
	int32_t type;			// setup(0),push(1),pull(2),ack(3),err(4)
	// 
	// default 0 
	// setup: timebase=1000/fps
	// push,pull: more fragment
	// 
	int32_t reserved;
	int32_t MP;				// more packet?
	uint32_t seq;			// sequence number
	uint64_t timestamp;		// send time
	char app[ 64 ];		// app
};
#pragma pack()

struct PACKET
{
	HEADER header;
	char body[ MAX_BODY_SIZE ];
};

// raw packet: PACKET
// raw net packet: PACKET(in net byte order)
// packet: Packet
class Packet
{
public:
	Packet( const uint32_t& size,
			const int32_t& type,
			const int32_t& reserved,
			const int32_t& MP,
			const uint32_t& seq,
			const uint64_t& timestamp,
			const std::string& app,
			const char *body );
	Packet( char *rawNetPacket );
	Packet( PACKET *rawPacket ) { memcpy( &fRawPacket, rawPacket, MAX_PACKET_SIZE ); }
	~Packet();
	PACKET raw_packet() { return fRawPacket; }
	PACKET raw_net_packet();
	PACKET* alloc_raw_packet();
	uint32_t size() const { return fRawPacket.header.size; }
	int32_t type() const { return fRawPacket.header.type; }
	int32_t reserved() const { return fRawPacket.header.reserved; }
	int32_t timebase() const { return fRawPacket.header.reserved; }
	int32_t MP() const { return fRawPacket.header.MP; }
	uint32_t seq() const { return fRawPacket.header.seq; }
	uint64_t timestamp() const { return fRawPacket.header.timestamp; }
	std::string app() const { return fRawPacket.header.app; }
	const char* body() const { return fRawPacket.body; }
	uint32_t packet_size() const { return PACK_SIZE_H( fRawPacket.header ); }
	uint32_t body_size() const { return BODY_SIZE_H( fRawPacket.header ); }
private:
	PACKET fRawPacket;
};

class PacketUtils
{
public:
	static Packet* new_packet( const Packet& packet )
	{
		return new Packet( packet.size(), packet.type(), packet.reserved(), packet.MP(), packet.seq(), packet.timestamp(), packet.app(), packet.body() );
	}
	static Packet ack_packet( const std::string& app )
	{
		return Packet( 0, Ack, 0, 0, 0, get_timestamp_ms(), app, nullptr );
	}
	static Packet* new_ack_packet( const std::string& app )
	{
		return new Packet( 0, Ack, 0, 0, 0, get_timestamp_ms(), app, nullptr );
	}
	static Packet build_connect_packet( const uint32_t& size, const std::string& app, const char* body )
	{
		return Packet( size, BuildConnect, 0, 0, 0, get_timestamp_ms(), app, body );
	}
	static Packet* new_build_connect_packet( const uint32_t& size, const std::string& app, const char* body )
	{
		return new Packet( size, BuildConnect, 0, 0, 0, get_timestamp_ms(), app, body );
	}
	static Packet accept_packet( const uint32_t& size, const std::string& app, const int32_t& timebase, const char* body )
	{
		return Packet( size, Accept, timebase, 0, 0, get_timestamp_ms(), app, body );
	}
	static Packet* new_accept_packet( const uint32_t& size, const std::string& app, const int32_t& timebase, const char* body )
	{
		return new Packet( size, Accept, timebase, 0, 0, get_timestamp_ms(), app, body );
	}
	static Packet err_packet( const std::string& app )
	{
		return Packet( 0, Err, 0, 0, 0, get_timestamp_ms(), app, nullptr );
	}
	static Packet* new_err_packet( const std::string& app )
	{
		return new Packet( 0, Err, 0, 0, 0, get_timestamp_ms(), app, nullptr );
	}
	static Packet pull_packet( const uint32_t& size, const int32_t& MP, const uint32_t& seq, const uint64_t& timestamp, const std::string& app, const char* body )
	{
		return Packet( size, Pull, 0, MP, seq, timestamp, app, body );
	}
	static Packet* new_pull_packet( const uint32_t& size, const int32_t& MP, const uint32_t& seq, const uint64_t& timestamp, const std::string& app, const char* body )
	{
		return new Packet( size, Pull, 0, MP, seq, timestamp, app, body );
	}
	static Packet fin_packet( const uint64_t& timestamp, const std::string& app )
	{
		return Packet( 0, Fin, 0, 0, 0, timestamp, app, nullptr );
	}
	static Packet* new_fin_packet( const uint64_t& timestamp, const std::string& app )
	{
		return new Packet( 0, Fin, 0, 0, 0, timestamp, app, nullptr );
	}
	static Packet create_stream_packet( const uint64_t& timestamp, const std::string& app, const int32_t& timebase )
	{
		return Packet( 0, CreateStream, timebase, 0, 0, timestamp, app, nullptr );
	}
	static Packet* new_create_stream_packet( const uint64_t& timestamp, const std::string& app, const int32_t& timebase )
	{
		return new Packet( 0, CreateStream, timebase, 0, 0, timestamp, app, nullptr );
	}
	static Packet push_packet( const uint32_t& size, const int32_t& MP, const uint32_t& seq, const uint64_t& timestamp, const std::string& app, const char* body )
	{
		return Packet( size, Push, 0, MP, seq, timestamp, app, body );
	}
	static Packet* new_push_packet( const uint32_t& size, const int32_t& MP, const uint32_t& seq, const uint64_t& timestamp, const std::string& app, const char* body )
	{
		return new Packet( size, Push, 0, MP, seq, timestamp, app, body );
	}
	static Packet new_session_packet( const uint64_t& timestamp, const std::string& app )
	{
		return Packet( 0, NewSession, 0, 0, 0, timestamp, app, nullptr );
	}
	static Packet* new_new_session_packet( const uint64_t& timestamp, const std::string& app )
	{
		return new Packet( 0, NewSession, 0, 0, 0, timestamp, app, nullptr );
	}
	static Packet lost_session_packet( const uint64_t& timestamp, const std::string& app )
	{
		return Packet( 0, LostSession, 0, 0, 0, timestamp, app, nullptr );
	}
	static Packet* new_lost_session_packet( const uint64_t& timestamp, const std::string& app )
	{
		return new Packet( 0, LostSession, 0, 0, 0, timestamp, app, nullptr );
	}
	static Packet online_sessions_packet( const uint32_t& size, const int32_t& MP, const uint32_t& seq, const uint64_t& timestamp, const std::string& app, const char* body )
	{
		return Packet( size, OnlineSessions, 0, MP, seq, timestamp, app, body );
	}
	static Packet* new_online_sessions_packet( const uint32_t& size, const int32_t& MP, const uint32_t& seq, const uint64_t& timestamp, const std::string& app, const char* body )
	{
		return new Packet( size, OnlineSessions, 0, MP, seq, timestamp, app, body );
	}
};

inline PACKET* Packet::alloc_raw_packet()
{
	uint32_t packetSize = PACK_SIZE_H( fRawPacket.header );
	PACKET *rawPacket = ( PACKET * ) malloc( packetSize );
	memcpy( rawPacket, &fRawPacket, packetSize );
	return rawPacket;
}

inline PACKET Packet::raw_net_packet()
{
	PACKET netPkt;
	netPkt = fRawPacket;
	netPkt.header.size = htonl( netPkt.header.size );
	netPkt.header.type = htonl( netPkt.header.type );
	netPkt.header.reserved = htonl( netPkt.header.reserved );
	netPkt.header.MP = htonl( netPkt.header.MP );
	netPkt.header.seq = htonl( netPkt.header.seq );
	netPkt.header.timestamp = htonll( netPkt.header.timestamp );
	//strncpy( netPkt.header.app, fRawPacket.header.app, sizeof fRawPacket.header.app );
	return netPkt;
}
