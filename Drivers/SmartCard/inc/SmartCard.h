
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
#include <interfaces/timer.h>
#include <devices/timer.h>

// --

#define CCID_HEADER 10U
#define CCID_MAX_MESSAGE ( 65544U + CCID_HEADER )

#define CCID_IO_US 2000000U
#define CCID_COMMAND_US 30000000U
#define CCID_RECOVERY_US 5000000U
#define CCID_TIME_UNIT_US 1000000U

struct SmartCardReader
{
	struct USB2SmartCardInfo Info;
	struct USB2_Register *Registration;
	struct USB2_EPResource *In;
	struct USB2_EPResource *Out;
	struct USB2IFace *USB;
	struct TimeRequest *ClockRequest;
	struct TimerIFace *Clock;
	struct MsgPort *Commands;
	U8 *Buffer;
	U32 Instance;
	U32 ReceiveSize;
	U8 Interface;
	U8 Sequence;
	U8 Stopped;
	U8 Faulted;
};

struct SmartCardDriver
{
	struct Library Library;
	struct SignalSemaphore Lock;
	struct SmartCardReader *Readers[USB2SMARTCARD_UNITS];
	U32 NextInstance;
};

struct SmartCardCommand
{
	struct Message Message;
	struct USB2SmartCardReply Reply;
	U8 Input[USB2SMARTCARD_APDU_MAX];
	U8 Output[USB2SMARTCARD_APDU_RESPONSE_MAX];
	ULONG InputSize;
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
LONG SmartCard_GetInstance( struct USB2SmartCardIFace *Self, ULONG unit,
	struct USB2SmartCardInfo *info, ULONG *instance );
LONG SmartCard_CommandInstance( struct USB2SmartCardIFace *Self, ULONG unit, ULONG instance, ULONG slot,
	enum USB2SmartCardOperation operation, CONST_APTR input, ULONG inputSize,
	APTR output, ULONG outputSize, struct USB2SmartCardReply *reply );
void SmartCard_Notice( struct SmartCardReader *reader, U32 signals );
U32 SmartCard_Now( struct SmartCardReader *reader );
S32 SmartCard_Transfer( struct SmartCardReader *reader, struct USB2_EPResource *endpoint,
	U16 operation, U8 *buffer, U32 length, U32 start, U32 budget, U32 timeout, U32 *actual );
LONG SmartCard_Receive( struct SmartCardReader *reader, U8 slot, U8 sequence, U8 expected,
	U32 start, U32 budget, BOOL recovering, U32 *length, S32 *usbError );
BOOL SmartCard_Recover( struct SmartCardReader *reader, struct USB2_EPResource *endpoint,
	S32 usbError, U8 slot, U8 sequence, U8 expected, BOOL abortable );
LONG SmartCard_Exchange( struct SmartCardReader *reader, struct SmartCardCommand *command );

// --

static inline U32
CCID_Read32( const U8 *data )
{
	return ( (U32) data[0] | ( (U32) data[1] << 8 ) |
		( (U32) data[2] << 16 ) | ( (U32) data[3] << 24 ) );
}

#endif
