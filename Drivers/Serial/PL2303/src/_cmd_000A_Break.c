/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Cmd_Break( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
U32 ticks;
U16 ifcnr;

	if ( ( unit->ACM ) && ( ! ( unit->ACMCapabilities & SERIAL_ACM_BREAK ) ) )
	{
		ioreq->IOSer.io_Error = IOERR_NOCMD;

		Serial_Reply( ctx, ioreq );

		return;
	}

	ifcnr = unit->StartMsg->InterfaceDescriptor->InterfaceNumber;
	ticks = ioreq->io_BrkTime / 20000;

	if ( ioreq->io_BrkTime % 20000 )
	{
		ticks++;
	}

	if ( ! ticks )
	{
		ticks = 1;
	}

	if ( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x23, 0xffff, ifcnr, NULL, 0 ) )
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;
		Serial_Reply( ctx, ioreq );

		return;
	}

	Delay( ticks );

	if ( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x23, 0, ifcnr, NULL, 0 ) )
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;
	}

	Serial_Reply( ctx, ioreq );
}

// --
