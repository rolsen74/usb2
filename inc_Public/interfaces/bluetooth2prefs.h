/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef BLUETOOTH2_PREFS_H
#define BLUETOOTH2_PREFS_H

#include "Bluetooth.h"

enum BT2PrefsState
{
	BT2_PREFS_IDLE,
	BT2_PREFS_CONNECTING,
	BT2_PREFS_CONNECTED,
	BT2_PREFS_DISCONNECTING,
};

struct BT2PrefsAdapter
{
	uint32_t Unit;
	uint32_t State;
	uint32_t LastResult;
	uint8_t Address[6];
	uint8_t Peer[6];
	uint8_t Ready;
	uint8_t HID;
	uint8_t Busy;
};

struct BT2PrefsPeer
{
	uint8_t Address[6];
	uint8_t AutoConnect;
};

enum BT2PrefsAction
{
	BT2_PREFS_CONNECT,
	BT2_PREFS_DISCONNECT,
	BT2_PREFS_FORGET,
	BT2_PREFS_AUTO_ON,
	BT2_PREFS_AUTO_OFF,
};

#endif
