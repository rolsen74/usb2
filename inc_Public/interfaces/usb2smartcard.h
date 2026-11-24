
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INTERFACES_USB2SMARTCARD_H
#define INTERFACES_USB2SMARTCARD_H

#include <exec/interfaces.h>

// --

#define USB2SMARTCARD_DRIVER "cls_SmartCard.Driver"
#define USB2SMARTCARD_INTERFACE "ccid"
#define USB2SMARTCARD_VERSION 1
#define USB2SMARTCARD_INSTANCE_INTERFACE "ccid-instance"
#define USB2SMARTCARD_INSTANCE_VERSION 2
#define USB2SMARTCARD_UNITS 32
#define USB2SMARTCARD_APDU_MAX 261
#define USB2SMARTCARD_APDU_RESPONSE_MAX 258
#define USB2SMARTCARD_ATR_MAX 33
#define USB2SMARTCARD_SHORT_APDU 0x00020000UL
#define USB2SMARTCARD_EXTENDED_APDU 0x00040000UL

enum USB2SmartCardOperation
{
	USB2SCC_Status,
	USB2SCC_PowerOn,
	USB2SCC_PowerOff,
	USB2SCC_Exchange
};

enum USB2SmartCardResult
{
	USB2SC_Okay,
	USB2SC_NoReader,
	USB2SC_BadArgument,
	USB2SC_NoMemory,
	USB2SC_USBError,
	USB2SC_ProtocolError,
	USB2SC_ReaderError,
	USB2SC_BufferTooSmall,
	USB2SC_Unsupported
};

enum USB2SmartCardState
{
	USB2SC_Active,
	USB2SC_Inactive,
	USB2SC_NoCard,
	USB2SC_Unknown
};

struct USB2SmartCardInfo
{
	ULONG MaxMessage;
	ULONG Features;
	UWORD Vendor;
	UWORD Product;
	UWORD Slots;
};

struct USB2SmartCardReply
{
	ULONG Length;
	UBYTE State;
	UBYTE ReaderError;
};

/*
 * Calls are synchronous. A reader's slots share one USB command queue.
 * PowerOn and Exchange require buffers of ATR_MAX and APDU_RESPONSE_MAX
 * respectively. BufferTooSmall is returned before submitting the command.
 * A USB or protocol error may leave the result of a dispatched APDU unknown.
 * Version 1 unit numbers select the current reader. Version 2 callers can
 * bind an individual command to the instance returned by GetInstance.
 */
struct USB2SmartCardIFace
{
	struct InterfaceData Data;
	ULONG APICALL (*Obtain)( struct USB2SmartCardIFace *Self );
	ULONG APICALL (*Release)( struct USB2SmartCardIFace *Self );
	void APICALL (*Expunge)( struct USB2SmartCardIFace *Self );
	struct Interface *APICALL (*Clone)( struct USB2SmartCardIFace *Self );

	LONG APICALL (*GetInfo)( struct USB2SmartCardIFace *Self, ULONG unit, struct USB2SmartCardInfo *info );
	LONG APICALL (*Command)( struct USB2SmartCardIFace *Self, ULONG unit, ULONG slot,
		enum USB2SmartCardOperation operation, CONST_APTR input, ULONG inputSize,
		APTR output, ULONG outputSize, struct USB2SmartCardReply *reply );

	/* Version 2 */
	LONG APICALL (*GetInstance)( struct USB2SmartCardIFace *Self, ULONG unit,
		struct USB2SmartCardInfo *info, ULONG *instance );
	LONG APICALL (*CommandInstance)( struct USB2SmartCardIFace *Self, ULONG unit, ULONG instance,
		ULONG slot, enum USB2SmartCardOperation operation, CONST_APTR input, ULONG inputSize,
		APTR output, ULONG outputSize, struct USB2SmartCardReply *reply );
};

#endif
