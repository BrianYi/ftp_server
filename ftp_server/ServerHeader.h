/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "PlatformHeader.h"
#include "Log.h"

/*
 * default send and receive buffer size
 * but not always this, you can change it by:
 * set_socket_rcvbuf_size
 * set_socket_sndbuf_size
 */
const uint32_t RECV_BUF_SIZE = 10 * 1024u;
const uint32_t SEND_BUF_SIZE = 10 * 1024u;

#define SERVER_VERSION		"0.0.1"
#define SERVER_IP			"192.168.1.107"
#define SERVER_DATA_PORT	20
#define SERVER_COMMAND_PORT	21

#define TASK_TIMEOUT		300

/* every read or write task time spending should be limited to this ms */
#define DEBUG_RW_TIME_MAX	10

#define DEBUG_Queue 0
#if DEBUG_Queue
#define DEBUG_Queue_OTHER 0
#endif

#define DEBUG_DataTransferSession	0
#if DEBUG_DataTransferSession
#define DEBUG_DataTransferSession_OTHER		0
#define DEBUG_DataTransferSession_RD_TIME	1
#define DEBUG_DataTransferSession_WR_TIME	1
#endif

#define DEBUG_FTPSession			0
#if DEBUG_FTPSession
#define DEBUG_FTPSession_OTHER		0
#define DEBUG_FTPSession_RD_TIME	1
#define DEBUG_FTPSession_WR_TIME	1
#endif

#define DEBUG_TaskThread			0
#if DEBUG_TaskThread
#define DEBUG_TaskThread_OTHER		1
#endif