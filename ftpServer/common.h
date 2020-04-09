#pragma once
#include <stdio.h>
#if __linux__
#include <stdint-gcc.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <endian.h>
#include <string.h>
#include <string>
#endif

#if __linux__
#define ntohll		be64toh
#define htonll		htobe64
#define msleep(ms)	usleep(1000 * ms)
#endif


inline uint64_t get_timestamp_ms( )
{
	struct timeval tv;
	gettimeofday( &tv, nullptr );
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}