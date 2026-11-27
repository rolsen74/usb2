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

SEC_CODE S32
Serial_ApplyParams( struct SerialContext *ctx, struct SerialUnit *unit, const struct IOExtSer *params )
{
	U8 line[7];
	U32 baud;
	U32 index;
	BOOL changed;

	baud = params->io_Baud;

	if ( ( ! baud ) || ( params->io_ReadLen != params->io_WriteLen ) ||
		( params->io_ReadLen < 5 ) || ( params->io_ReadLen > 8 ) ||
		( params->io_StopBits < 1 ) || ( params->io_StopBits > 2 ) ||
		( params->io_SerFlags & SERF_7WIRE ) )
	{
		return ( SerErr_InvParam );
	}

	if ( ! SupportedRate( unit, baud ) )
	{
		return ( SerErr_BaudMismatch );
	}

	line[0] = baud;
	line[1] = baud >> 8;
	line[2] = baud >> 16;
	line[3] = baud >> 24;
	line[4] = ( params->io_StopBits == 2 ) ? 2 : 0;
	line[5] = 0;
	line[6] = params->io_ReadLen;

	if ( params->io_SerFlags & SERF_PARTY_ON )
	{
		line[5] = ( params->io_SerFlags & SERF_PARTY_ODD ) ? 1 : 2;
	}

	if ( params->io_ExtFlags & SEXTF_MSPON )
	{
		line[5] = ( params->io_ExtFlags & SEXTF_MARK ) ? 3 : 4;
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
			return ( SerErr_InvParam );
		}

		if ( ( unit->WriteActive ) || ( unit->BreakActive ) ||
			( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
				0x20, 0, unit->ACM ? unit->StartMsg->InterfaceDescriptor->InterfaceNumber : 0, line, sizeof( line ) ) ) )
		{
			return ( SerErr_DevBusy );
		}

		CopyMem( line, unit->Line, sizeof( line ) );
	}

	return ( 0 );
}

// --

SEC_CODE void
Serial_Cmd_SetParams( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	ioreq->IOSer.io_Error = Serial_ApplyParams( ctx, unit, ioreq );

	if ( ( ! ioreq->IOSer.io_Error ) && ( unit->Sessions ) )
	{
		unit->PortInterface->SetParams( unit->PortUnit, ioreq );
	}

	Serial_Reply( ctx, ioreq );
}

// --
