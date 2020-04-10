#include "Packet.h"

Packet::Packet( char *rawNetPacket )
{
	memcpy( &fRawPacket, rawNetPacket, MAX_PACKET_SIZE );

	// to host byte
	fRawPacket.header.size = ntohl( fRawPacket.header.size );
	fRawPacket.header.type = ntohl( fRawPacket.header.type );
	fRawPacket.header.reserved = ntohl( fRawPacket.header.reserved );
	fRawPacket.header.MP = ntohl( fRawPacket.header.MP );
	fRawPacket.header.seq = ntohl( fRawPacket.header.seq );	
	fRawPacket.header.timestamp = ntohll( fRawPacket.header.timestamp );

	// app and body needn't to change
}

Packet::Packet( const uint32_t& size, const int32_t& type, const int32_t& reserved, const int32_t& MP, const uint32_t& seq, const uint64_t& timestamp, const std::string& app, const char *body )
{
	fRawPacket.header.size = size;
	fRawPacket.header.type = type;
	fRawPacket.header.reserved = reserved;
	fRawPacket.header.MP = MP;
	fRawPacket.header.seq = seq;
	fRawPacket.header.timestamp = timestamp;
	strcpy( fRawPacket.header.app, app.c_str( ) );

	if ( body )
		memcpy( fRawPacket.body, (void *)body, BODY_SIZE_H( fRawPacket.header ) );
}

Packet::~Packet( )
{
}
