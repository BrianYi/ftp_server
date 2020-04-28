/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#pragma once
#include "PlatformHeader.h"
#include "Log.h"

const uint32_t RECV_BUF_SIZE = 10 * 1024u;
const uint32_t SEND_BUF_SIZE = 10 * 1024u;

#define SERVER_VERSION		"0.0.1"
#define SERVER_IP			"0.0.0.0"
#define SERVER_DATA_PORT	20
#define SERVER_COMMAND_PORT	21