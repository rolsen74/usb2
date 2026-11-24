/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_RODATA static const U32 PL2303DirectRates[] =
{
	75, 150, 300, 600, 1200, 1800, 2400, 3600, 4800, 7200, 9600, 14400,
	19200, 28800, 38400, 57600, 115200, 230400, 460800, 614400, 921600,
	1228800, 2457600, 3000000, 6000000
};

// --

SEC_CODE static BOOL
SupportedRate( struct SerialUnit *unit, U32 baud )
{
	U32 index;

	if ( unit->ACM )
	{
		return ( TRUE );
	}

	if ( ( unit->PLType == SERIAL_PL_H ) && ( baud > 1228800 ) )
	{
		return ( FALSE );
	}

	for( index = 0; index < sizeof( PL2303DirectRates ) / sizeof( PL2303DirectRates[0] ); index++ )
	{
		if ( PL2303DirectRates[index] == baud )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

SEC_CODE void
Serial_Cmd_SetParams( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	U8 line[7];
	U32 baud;
	U32 index;
	BOOL changed;

	baud = ioreq->io_Baud;

	if ( ( ! baud ) || ( ioreq->io_ReadLen != ioreq->io_WriteLen ) ||
		( ioreq->io_ReadLen < 5 ) || ( ioreq->io_ReadLen > 8 ) ||
		( ioreq->io_StopBits < 1 ) || ( ioreq->io_StopBits > 2 ) ||
		( ioreq->io_SerFlags & SERF_7WIRE ) )
	{
		ioreq->IOSer.io_Error = SerErr_InvParam;

		goto reply;
	}

	if ( ! SupportedRate( unit, baud ) )
	{
		ioreq->IOSer.io_Error = SerErr_BaudMismatch;

		goto reply;
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

	changed = FALSE;

	for( index = 0; index < sizeof( line ); index++ )
	{
		if ( unit->Line[index] != line[index] )
		{
			changed = TRUE;

			break;
		}
	}

	if ( changed )
	{
		if ( ( unit->ACM ) && ( ! ( unit->ACMCapabilities & SERIAL_ACM_LINE ) ) )
		{
			ioreq->IOSer.io_Error = SerErr_InvParam;

			goto reply;
		}

		if ( ( unit->WriteActive ) || ( unit->BreakActive ) ||
			( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
				0x20, 0, unit->ACM ? unit->StartMsg->InterfaceDescriptor->InterfaceNumber : 0, line, sizeof( line ) ) ) )
		{
			ioreq->IOSer.io_Error = SerErr_DevBusy;

			goto reply;
		}

		CopyMem( line, unit->Line, sizeof( line ) );
	}

	if ( unit->SerialInterface )
	{
		unit->SerialInterface->SetParams( unit->PortUnit, ioreq );
	}

reply:
	Serial_Reply( ctx, ioreq );
}

// --
