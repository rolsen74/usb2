
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2WIFI_H
#define USB2WIFI_H

#include <dos/dos.h>
#include <exec/interfaces.h>
#include <stdint.h>

// --

#define USB2WIFI_NAME "usb2wifi.library"
#define USB2WIFI_VERSION 1
#define USB2WIFI_PROMPT_REVISION 3
#define USB2WIFI_PROFILE_REVISION 4
#define USB2WIFI_STARTUP_REVISION 5
#define USB2WIFI_RETRY_PICKER_REVISION 18
#define USB2WIFI_RTL8188 0
#define USB2WIFI_RT2870 1

#define USB2WIFI_OPEN 0
#define USB2WIFI_WPA 1
#define USB2WIFI_WPA2 2
#define USB2WIFI_WPA3 3

#define USB2WIFI_PROFILE_AUTO 1
#define USB2WIFI_PROFILE_HIDDEN 2

#define USB2WIFI_KEEP_PASSWORD 0
#define USB2WIFI_SAVE_PASSWORD 1
#define USB2WIFI_FORGET_PASSWORD 2

#define USB2WIFI_OK 1
#define USB2WIFI_MISSING 0
#define USB2WIFI_ERROR (-1)
#define USB2WIFI_CONNECT_NOW 2

struct USB2WiFiProfile
{
	uint8_t Security;
	uint8_t Channel;
	uint8_t Options;
	char SSID[33];
	char Password[64];
};

// The caller retains the library and this request until DOS replies to Death.
struct USB2WiFiPrompt
{
	struct DeathMessage Death;
	struct Task *Worker;
	uint32_t Cancelled;
	int32_t Result;
	uint32_t PasswordAction;
	uint8_t Security;
	uint8_t Retry;
	char SSID[33];
	char Password[64];
};

struct USB2WiFiIFace
{
	struct InterfaceData Data;
	uint32_t APICALL (*Obtain)( struct USB2WiFiIFace *self );
	uint32_t APICALL (*Release)( struct USB2WiFiIFace *self );
	APTR Reserved[2];
	int32_t APICALL (*Load)( struct USB2WiFiIFace *self, uint32_t adapter, uint32_t unit, struct USB2WiFiProfile *profile );
	int32_t APICALL (*Save)( struct USB2WiFiIFace *self, uint32_t adapter, uint32_t unit, const struct USB2WiFiProfile *profile,
		uint32_t passwordAction );
	int32_t APICALL (*Connected)( struct USB2WiFiIFace *self, uint32_t adapter, uint32_t unit, const struct USB2WiFiProfile *profile,
		uint32_t savePassword );
	int32_t APICALL (*Claim)( struct USB2WiFiIFace *self, uint32_t adapter, uint32_t unit );
	void APICALL (*Unclaim)( struct USB2WiFiIFace *self, uint32_t adapter, uint32_t unit );
	int32_t APICALL (*Setup)( struct USB2WiFiIFace *self, struct USB2WiFiProfile *profile, uint32_t adapter, uint32_t unit,
		uint32_t *action );
	int32_t APICALL (*StartPrompt)( struct USB2WiFiIFace *self, struct USB2WiFiPrompt *prompt );
	void APICALL (*CancelPrompt)( struct USB2WiFiIFace *self, struct USB2WiFiPrompt *prompt );
	int32_t APICALL (*StartAuto)( struct USB2WiFiIFace *self, uint32_t adapter, uint32_t unit );
	int32_t APICALL (*SetupRetry)( struct USB2WiFiIFace *self, struct USB2WiFiProfile *profile,
		uint32_t adapter, uint32_t unit, uint32_t *action );
};

#endif
