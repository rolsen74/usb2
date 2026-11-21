/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Cmd_SetParams( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
U8 line[7];
U32 baud;
U32 index;
S32 line_control;

	baud = ioreq->io_Baud;

	if ( ( ! baud ) || ( ioreq->io_ReadLen != ioreq->io_WriteLen ) ||
		( ioreq->io_ReadLen < 5 ) || ( ioreq->io_ReadLen > 8 ) ||
		( ioreq->io_StopBits < 1 ) || ( ioreq->io_StopBits > 2 ) )
	{
		ioreq->IOSer.io_Error = SerErr_InvParam;
		Serial_Reply( ctx, ioreq );

		return;
	}

	line[0] = baud;
	line[1] = baud >> 8;
	line[2] = baud >> 16;
	line[3] = baud >> 24;
	line[4] = ( ioreq->io_StopBits == 2 ) ? 2 : 0;
	line[5] = 0;
	line[6] = ioreq->io_ReadLen;

	if ( ioreq->io_SerFlags & SERF_PARTY_ON )
	{
		line[5] = ( ioreq->io_SerFlags & SERF_PARTY_ODD ) ? 1 : 2;
	}

	if ( ioreq->io_ExtFlags & SEXTF_MSPON )
	{
		line[5] = ( ioreq->io_ExtFlags & SEXTF_MARK ) ? 3 : 4;
	}

	line_control = ( ! unit->ACM ) || ( unit->ACMCapabilities & SERIAL_ACM_LINE );

	if ( ( line_control ) && ( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x20, 0, unit->StartMsg->InterfaceDescriptor->InterfaceNumber, line, 7 ) ) )
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;
		Serial_Reply( ctx, ioreq );

		return;
	}

	for( index = 0; index < 7; index++ )
	{
		unit->Line[index] = line[index];
	}

	// DTR and RTS asserted while the port is configured.
	if ( ( line_control ) && ( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x22, 3, unit->StartMsg->InterfaceDescriptor->InterfaceNumber, NULL, 0 ) ) )
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;
	}

	Serial_Reply( ctx, ioreq );
}

// --
