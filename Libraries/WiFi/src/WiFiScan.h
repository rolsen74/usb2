
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_WIFI_SCAN_H
#define USB2_WIFI_SCAN_H

#include <stdint.h>

#define WIFI_SCAN_NETWORKS 48
#define WIFI_SECURITY_UNSUPPORTED 255

struct WiFiNetwork
{
	char SSID[33];
	uint8_t Security;
	uint8_t SecurityModes;
	uint8_t Channel;
};

struct WiFiScan
{
	struct WiFiNetwork Network[WIFI_SCAN_NETWORKS];
	uint32_t Count;
	uint32_t Changed;
	uint32_t InputMask;
	int32_t (*Update)( void *context );
	void *Context;
};

int32_t WiFi_PickNetwork( uint32_t adapter, uint32_t unit, struct WiFiScan *scan );

#endif
