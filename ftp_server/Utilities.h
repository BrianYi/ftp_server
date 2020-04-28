#pragma once
#include "ServerHeader.h"
#include <pwd.h>
#include <shadow.h>

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


inline int sys_auth_user( const char*username, const char*password )
{
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
