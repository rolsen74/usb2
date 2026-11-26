
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef USB2_AF9015_H
#define USB2_AF9015_H

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/usb2.h>

#define AF9015_VENDOR  0x15a4
#define AF9015_PRODUCT 0x9016
#define AF9015_INTERFACE 0
#define AF9015_TS_IN 0x84
#define AF9015_TS_PACKET 188U
#define AF9015_TS_BLOCK ( 87U * AF9015_TS_PACKET )
#define AF9015_TDA18218 0xb3
#define AF9015_MT2060 0x82
#define AF9015_MT2060_2 0x93

struct Receiver
{
	struct USB2_Register *Reg;
	struct USB2_EPResource *Out;
	struct USB2_EPResource *In;

	U32 Clock;
	U32 IFrequency;
	U16 MT2060_IF1;
	U8 Tuner;
	U8 ClockType;
	U8 Dual;
	U8 Sequence;
	BOOL Detached;
	BOOL DemodAwake;
	BOOL TunerDetected;
};

struct RegisterSetting
{
	U16 Address;
	U8 Mask;
	U8 Value;
};

BOOL AF9015_Open( struct Receiver *receiver );
void AF9015_Close( struct Receiver *receiver );
BOOL AF9015_Detached( struct Receiver *receiver );
BOOL AF9015_Transfer( struct USB2_EPResource *ep, BOOL read, U32 size, BOOL shortOK );
BOOL AF9015_Firmware( struct Receiver *receiver, const char *path );
BOOL AF9015_Board( struct Receiver *receiver );
BOOL AF9015_Read( struct Receiver *receiver, U16 reg, U8 *data, U8 size );
BOOL AF9015_Write( struct Receiver *receiver, U16 reg, const U8 *data, U8 size );
BOOL AF9015_Set( struct Receiver *receiver, U16 reg, U8 mask, U8 value );
BOOL AF9015_Table( struct Receiver *receiver, const struct RegisterSetting *table, U32 count );
BOOL AF9015_I2CRead( struct Receiver *receiver, U8 chip, U8 reg, U8 *data, U8 size );
BOOL AF9015_I2CWrite( struct Receiver *receiver, U8 chip, U8 reg, const U8 *data, U8 size );
BOOL AF9015_DemodInit( struct Receiver *receiver );
BOOL AF9015_DemodTune( struct Receiver *receiver, U32 bandwidth );
void AF9015_DemodSleep( struct Receiver *receiver );
BOOL AF9015_Tune( struct Receiver *receiver, U32 frequency, U32 bandwidth );
BOOL TDA18218_Tune( struct Receiver *receiver, U32 frequency, U32 bandwidth );
BOOL MT2060_Tune( struct Receiver *receiver, U32 frequency );
void AF9015_TunerSleep( struct Receiver *receiver );
BOOL AF9015_Capture( struct Receiver *receiver, const char *path, U32 blocks );

#endif
