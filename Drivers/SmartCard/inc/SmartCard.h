
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef DRIVERS_SMARTCARD_H
#define DRIVERS_SMARTCARD_H

#include <proto/exec.h>
#include <interfaces/usb2.h>
#include <interfaces/usb2smartcard.h>
#include <devices/usb2.h>

// --

#define CCID_HEADER 10U
#define CCID_MAX_MESSAGE ( 65544U + CCID_HEADER )

struct SmartCardReader
{
	struct USB2SmartCardInfo Info;
	struct USB2_Register *Registration;
	struct USB2_EPResource *In;
	struct USB2_EPResource *Out;
	struct MsgPort *Commands;
	U8 *Buffer;
	U8 Sequence;
};

struct SmartCardDriver
{
	struct Library Library;
	struct SignalSemaphore Lock;
	struct SmartCardReader *Readers[USB2SMARTCARD_UNITS];
};

struct SmartCardCommand
{
	struct Message Message;
	struct USB2SmartCardReply *Reply;
	CONST_APTR Input;
	PTR Output;
	ULONG InputSize;
	ULONG OutputSize;
	ULONG Slot;
	enum USB2SmartCardOperation Operation;
	LONG Result;
};

U32 SmartCard_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
S32 SmartCard_Parse( struct USB2_DriverMessage *msg, struct USB2SmartCardInfo *info );
LONG SmartCard_GetInfo( struct USB2SmartCardIFace *Self, ULONG unit, struct USB2SmartCardInfo *info );
LONG SmartCard_Command( struct USB2SmartCardIFace *Self, ULONG unit, ULONG slot,
	enum USB2SmartCardOperation operation, CONST_APTR input, ULONG inputSize,
	APTR output, ULONG outputSize, struct USB2SmartCardReply *reply );
LONG SmartCard_Exchange( struct SmartCardReader *reader, struct SmartCardCommand *command );

// --

static inline U32
CCID_Read32( const U8 *data )
{
	return ( (U32) data[0] | ( (U32) data[1] << 8 ) |
		( (U32) data[2] << 16 ) | ( (U32) data[3] << 24 ) );
}

#endif
