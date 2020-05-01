/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#include "Packet.h"

// std::unordered_map<int, std::string> g_replyStr
// {
// 	std::make_pair( 200, "Command okay.\n" ),
// 	std::make_pair( 500, "Syntax error, command unrecognized.\n" ),
// 	std::make_pair( 501, "Syntax error in parameters or arguments.\n" ),
// 	std::make_pair( 202, "Command not implemented, superfluous at this site.\n" ),
// 	std::make_pair( 502, "Command not implemented.\n"),
// 	std::make_pair( 503, "Bad sequence of commands.\n"),
// 	std::make_pair( 504, "Command not implemented for that parameter.\n"),
// 	std::make_pair( 110, "Restart marker reply.\n"),
// 	std::make_pair( 211, "System status, or system help reply.\n"),
// 	std::make_pair( 212, "Directory status.\n"),
// 	std::make_pair( 213, "File status.\n"),
// 	std::make_pair( 214, "Help message.\n"),
// 	std::make_pair( 215, "UNIX\n"),
// 	std::make_pair( 120, "Service ready in nnn minutes.\n"),
// 	std::make_pair( 220, "Service ready for new user.(written by BrianYi)\n"),
// 	std::make_pair( 221, "Goodbye\n"),
// 	std::make_pair( 421, "Service not available, closing control connection.\n"),
// 	std::make_pair( 125, "Data connection already open; transfer starting.\n"),
// 	std::make_pair( 225, "Data connection open; no transfer in progress.\n"),
// 	std::make_pair( 425, "Can't open data connection.\n"),
// 	std::make_pair( 226, "Closing data connection.\n"),
// 	std::make_pair( 426, "Connection closed; transfer aborted.\n"),
// 	std::make_pair( 227, "Entering Passive Mode( h1, h2, h3, h4, p1, p2 ).\n"),
// 	std::make_pair( 230, "User logged in, proceed.\n"),
// 	std::make_pair( 530, "Login or password incorrect!\n"),
// 	std::make_pair( 331, "User name okay, need password.\n"),
// 	std::make_pair( 332, "Need account for login.\n"),
// 	std::make_pair( 532, "Need account for storing files.\n"),
// 	std::make_pair( 150, "File status okay; about to open data connection.\n"),
// 	std::make_pair( 250, "Requested file action okay, completed.\n"),
// 	std::make_pair( 257, "'PATHNAME' created.\n"),
// 	std::make_pair( 350, "Requested file action pending further information.\n"),
// 	std::make_pair( 450, "Requested file action not taken.\n"),
// 	std::make_pair( 550, "Requested action not taken.\n"),
// 	std::make_pair( 451, "Requested action aborted.Local error in processing.\n"),
// 	std::make_pair( 551, "Requested action aborted.Page type unknown.\n"),
// 	std::make_pair( 452, "Requested action not taken.\n"),
// 	std::make_pair( 552, "Requested file action aborted.\n"),
// 	std::make_pair( 553, "Requested action not taken.\n")
// };

#define STR( commType )	#commType
#define DEAL_COMMAND_STR(commStr,commType) \
	if (commStr == STR(commType)) return commType;

int CommandType( std::string& commStr )
{
	/*
	 * ACCESS CONTROL COMMANDS
	 */
	DEAL_COMMAND_STR( commStr, USER );	// USER NAME
	DEAL_COMMAND_STR( commStr, PASS );	// PASSWORD
	DEAL_COMMAND_STR( commStr, ACCT );	// ACCOUNT
	DEAL_COMMAND_STR( commStr, CWD );	// CHANGE WORKING DIRECTORY
	DEAL_COMMAND_STR( commStr, CDUP );	// CHANGE TO PARENT DIRECTORY
	DEAL_COMMAND_STR( commStr, SMNT );	// STRUCTURE MOUNT
	DEAL_COMMAND_STR( commStr, REIN );	// REINITIALIZE
	/*
	 * TRANSFER PARAMETER COMMANDS
	 */
	DEAL_COMMAND_STR( commStr, QUIT );	// LOGOUT
	DEAL_COMMAND_STR( commStr, PORT );	// DATA PORT
	DEAL_COMMAND_STR( commStr, PASV );	// PASSIVE
	DEAL_COMMAND_STR( commStr, TYPE );	// REPRESENTATION TYPE
	DEAL_COMMAND_STR( commStr, STRU );	// FILE STRUCTURE
	DEAL_COMMAND_STR( commStr, MODE );	// TRANSFER MODE
	/*
	 * FTP SERVICE COMMANDS
	 */
	DEAL_COMMAND_STR( commStr, RETR );	// RETRIEVE
	DEAL_COMMAND_STR( commStr, STOR );	// STORE
	DEAL_COMMAND_STR( commStr, STOU );	// STORE UNIQUE
	DEAL_COMMAND_STR( commStr, APPE );	// APPEND(with create)
	DEAL_COMMAND_STR( commStr, ALLO );	// ALLOCATE
	DEAL_COMMAND_STR( commStr, REST );	// RESTART
	DEAL_COMMAND_STR( commStr, RNFR );	// RENAME FROM
	DEAL_COMMAND_STR( commStr, RNTO );	// RENAME TO
	DEAL_COMMAND_STR( commStr, ABOR );	// ABORT
	DEAL_COMMAND_STR( commStr, DELE );	// DELETE
	DEAL_COMMAND_STR( commStr, RMD );	// REMOVE DIRECTORY
	DEAL_COMMAND_STR( commStr, MKD );	// MAKE DIRECTORY
	DEAL_COMMAND_STR( commStr, PWD );	// PRINT WORKING DIRECTORY
	DEAL_COMMAND_STR( commStr, LIST );	// LIST
	DEAL_COMMAND_STR( commStr, NLIST );	// NAME LIST
	DEAL_COMMAND_STR( commStr, SITE );	// SITE PARAMETERS
	DEAL_COMMAND_STR( commStr, SYST );	// SYSTEM
	DEAL_COMMAND_STR( commStr, STAT );	// STATUS
	DEAL_COMMAND_STR( commStr, HELP );	// HELP
	DEAL_COMMAND_STR( commStr, NOOP );	// NOOP
	return UNKNOWN;
}


Packet::Packet( const uint32_t& bodySize, const int32_t& type, const uint64_t& timestamp, const char *body, const bool& mp )
{
	fRawPacket.header.bodySize = bodySize;
	fRawPacket.header.type = type;
	fRawPacket.header.timestamp = timestamp;
	fRawPacket.header.MP = mp;
	if ( body )
		memcpy( fRawPacket.body, (void *)body, bodySize );
}

Packet::~Packet( )
{
}
