/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "ServerHeader.h"
#include <pwd.h>
#include <shadow.h>

#define __LINEINFO__ \
	std::string(__FILE__) + ":" +  std::to_string(__LINE__)

inline std::string ls_command( std::string dir )
{
	FILE *fp;
	char path[2048];
	std::string output;

	/* Open the command for reading. */
	std::string commandStr = "/bin/ls -al " + dir;
	fp = popen( commandStr.c_str(), "r" );
	if ( fp == NULL ) {
		printf( "Failed to run command\n" );
		exit( 1 );
	}

	/* Read the output a line at a time - output it. */
	while ( fgets( path, sizeof( path ), fp ) != NULL )
	{
		output += path;
	}

	/* close */
	pclose( fp );
	return output;
}

inline std::vector<std::string> SplitString( std::string sourStr,
	std::string delimiter )
{
	std::vector<std::string> strArry;
	size_t pos = 0;
	std::string token;
	while ( (pos = sourStr.find( delimiter )) != std::string::npos )
	{
		token = sourStr.substr( 0, pos );
		strArry.push_back( token );
		sourStr.erase( 0, pos + delimiter.length() );
	}
	strArry.push_back( sourStr );
	return strArry;
}

static std::mutex g_mux;
inline int sys_auth_user( const char*username, const char*password )
{
	std::unique_lock<std::mutex> locker( g_mux );
	struct passwd*pw;
	struct spwd*sp;
	char*encrypted, *correct;

	pw = getpwnam( username );
	endpwent();

	if ( !pw ) return 1; //user doesn't really exist

	sp = getspnam( pw->pw_name );
	endspent();
	if ( sp )
		correct = sp->sp_pwdp;
	else
		correct = pw->pw_passwd;

	encrypted = crypt( password, correct );
	return strcmp( encrypted, correct ) ? 2 : 0;  // bad pw=2, success=0
}

/*
 * this class can be used to estimate operation time
 * for example:
 * {
 *    DebugTime dbgTime(DebugTime::Print, __LINEINFO__);
 *    float y = 0.0f;
 *    float x = 2.0f;
 *    for (int i = 0; i < 100; ++i)
 *		y = sqrt(sqrt(pow(x,8.0)));
 * }
 */
class DebugTime
{
public:
	enum Type
	{
		Print,
		Assert
	};
	DebugTime( Type type, const std::string& msg )
	{
		fType = type; 
		fMsg = msg; 
		fCreateTimeMs = get_timestamp_ms();
		fMaxWasteTimeMs = 0;
	}
	~DebugTime() 
	{ 
		fDestroyTimeMs = get_timestamp_ms();
		uint64_t fDiffTimeMs = fDestroyTimeMs - fCreateTimeMs;
		fMaxWasteTimeMs = std::max( fMaxWasteTimeMs, fDiffTimeMs );
		switch ( fType )
		{
			case DebugTime::Print:
			{
				if (fMsg.empty() )
					printf( "time waste: %llu, max time waste: %llu\n", 
						fDiffTimeMs, fMaxWasteTimeMs );
				else
					printf( "time waste: %llu, max time waste: %llu, MSG:%s\n", 
						fDiffTimeMs, fMaxWasteTimeMs, fMsg.c_str() );
				break;
			}
			case DebugTime::Assert:
				assert( fDiffTimeMs <= DEBUG_RW_TIME_MAX );
				break;
			default:
				break;
		}
	}
private:
	uint64_t fCreateTimeMs;
	uint64_t fDestroyTimeMs;
	uint64_t fMaxWasteTimeMs;
	std::string fMsg;
	Type fType;
};