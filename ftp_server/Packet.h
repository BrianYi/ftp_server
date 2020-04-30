/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "PlatformHeader.h"

#if _WIN32
#define NOMINMAX
#include <WinSock2.h>
#else
#endif

#if __linux__
#define ntohll		be64toh
#define htonll		htobe64
#define msleep(ms)	usleep(1000 * ms)
#endif

#define MAX_BODY_SIZE 1400

enum FTPCommandType
{
	/*
	 * ACCESS CONTROL COMMANDS
	 */
	USER,	// USER NAME
	PASS,	// PASSWORD
	ACCT,	// ACCOUNT
	CWD,	// CHANGE WORKING DIRECTORY
	CDUP,	// CHANGE TO PARENT DIRECTORY
	SMNT,	// STRUCTURE MOUNT
	REIN,	// REINITIALIZE
	/*
	 * TRANSFER PARAMETER COMMANDS
	 */
	QUIT,	// LOGOUT
	PORT,	// DATA PORT
	PASV,	// PASSIVE
	TYPE,	// REPRESENTATION TYPE
	STRU,	// FILE STRUCTURE
	MODE,	// TRANSFER MODE
	/*
	 * FTP SERVICE COMMANDS
	 */
	RETR,	// RETRIEVE
	STOR,	// STORE
	STOU,	// STORE UNIQUE
	APPE,	// APPEND(with create)
	ALLO,	// ALLOCATE
	REST,	// RESTART
	RNFR,	// RENAME FROM
	RNTO,	// RENAME TO
	ABOR,	// ABORT
	DELE,	// DELETE
	RMD,	// REMOVE DIRECTORY
	MKD,	// MAKE DIRECTORY
	PWD,	// PRINT WORKING DIRECTORY
	LIST,	// LIST
	NLIST,	// NAME LIST
	SITE,	// SITE PARAMETERS
	SYST,	// SYSTEM
	STAT,	// STATUS
	HELP,	// HELP
	NOOP,	// NOOP
	UNKNOWN, //
	REPLY,	// 
	TypeNum
};

// 4+4+4+8+8+16=44
#pragma pack(1)
struct HEADER
{
	// total body size
	uint32_t bodySize;
	int32_t type;
	uint64_t timestamp;		// send time
	bool MP;		// more packet?
};
#pragma pack()

struct PACKET
{
	HEADER header;
	char body[MAX_BODY_SIZE];
};

// raw packet: PACKET
// raw net packet: PACKET(in net byte order)
// packet: Packet
class Packet
{
public:
	Packet( const uint32_t& bodySize, const int32_t& type,
		const uint64_t& timestamp,
		const char *body, const bool& mp);
	~Packet();
	int32_t type() const { return fRawPacket.header.type; }
	uint64_t timestamp() const { return fRawPacket.header.timestamp; }
	const char* body() const { return fRawPacket.body; }
	uint32_t body_size() const { return fRawPacket.header.bodySize; }
	bool more() { return fRawPacket.header.MP; }
private:
	PACKET fRawPacket;
};

extern std::unordered_map<int, std::string> g_replyStr;
int CommandType( std::string& commStr );

class PacketUtils
{
public:
// 	static Packet reply_packet( const int32_t& type, const int32_t& replyCode )
// 	{
// 		assert( g_replyStr.count( replyCode ) );
// 		std::string replyStr = g_replyStr[replyCode];
// 		return Packet( replyStr.size(), type, get_timestamp_ms(), replyStr.c_str(), 0 );
// 	}
// 	static Packet* new_reply_packet( const int32_t& type, const int32_t& replyCode )
// 	{
// 		assert( g_replyStr.count( replyCode ) );
// 		std::string replyStr = std::to_string(replyCode) + " " + g_replyStr[replyCode];
// 		return new Packet( replyStr.size(), type, get_timestamp_ms(), replyStr.c_str(), 0 );
// 	}
	static Packet packet( const int32_t& type, const char* body, const uint32_t& bodySize, const bool& mp)
	{
		return Packet( bodySize, type, get_timestamp_ms(), body, mp );
	}
	static Packet *new_packet( const int32_t& type, const char* body, const uint32_t& bodySize, const bool& mp )
	{
		return new Packet( bodySize, type, get_timestamp_ms(), body, mp );
	}
};
