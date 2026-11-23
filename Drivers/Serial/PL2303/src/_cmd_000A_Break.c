
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void
Serial_BreakInit( struct SerialUnit *unit )
{
	if ( ( unit->ACM ) && ( ! ( unit->ACMCapabilities & SERIAL_ACM_BREAK ) ) )
	{
		return;
	}

	unit->BreakPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! unit->BreakPort )
	{
		return;
	}

	unit->BreakTimer = AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( struct TimeRequest ), ASOIOR_ReplyPort, unit->BreakPort, TAG_END );

	if ( unit->BreakTimer )
	{
		unit->BreakTimerOpen = ( OpenDevice( "timer.device", UNIT_MICROHZ, (PTR)unit->BreakTimer, 0 ) == 0 );
	}
}

// --

SEC_CODE void
Serial_BreakStop( struct SerialContext *ctx, struct SerialUnit *unit, S32 error )
{
	struct IOExtSer *ioreq;

	U16 ifcnr;

	if ( ! unit->BreakActive )
	{
		return;
	}

	if ( unit->BreakTimerPending )
	{
		if ( ! CheckIO( (PTR)unit->BreakTimer ) )
		{
			AbortIO( (PTR)unit->BreakTimer );
		}

		WaitIO( (PTR)unit->BreakTimer );

		unit->BreakTimerPending = FALSE;
	}

	ifcnr = unit->StartMsg->InterfaceDescriptor->InterfaceNumber;

	if ( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x23, 0, ifcnr, NULL, 0 ) )
	{
		if ( ! error )
		{
			error = SerErr_DevBusy;
		}
	}

	ioreq = unit->BreakRequest;
	unit->BreakRequest = NULL;
	unit->BreakActive = FALSE;

	if ( ioreq )
	{
		ioreq->IOSer.io_Error = error;

		Serial_Reply( ctx, ioreq );
	}
}

// --

SEC_CODE void
Serial_BreakFree( struct SerialContext *ctx, struct SerialUnit *unit )
{
	Serial_BreakStop( ctx, unit, IOERR_ABORTED );

	if ( unit->BreakTimerOpen )
	{
		CloseDevice( (PTR)unit->BreakTimer );
	}

	if ( unit->BreakTimer )
	{
		FreeSysObject( ASOT_IOREQUEST, unit->BreakTimer );
	}

	if ( unit->BreakPort )
	{
		FreeSysObject( ASOT_PORT, unit->BreakPort );
	}
}

// --

SEC_CODE void
Serial_BreakDone( struct SerialContext *ctx, struct SerialUnit *unit )
{
	if ( GetMsg( unit->BreakPort ) )
	{
		unit->BreakTimerPending = FALSE;

		Serial_BreakStop( ctx, unit, unit->BreakTimer->Request.io_Error ? SerErr_DevBusy : 0 );

		Serial_Write_Start( ctx, unit );
	}
}

// --

SEC_CODE void
Serial_BreakStart( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	U32 micro;
	U16 ifcnr;

	ifcnr = unit->StartMsg->InterfaceDescriptor->InterfaceNumber;

	if ( ( unit->BreakActive ) || ( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x23, 0xffff, ifcnr, NULL, 0 ) ) )
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;

		Serial_Reply( ctx, ioreq );

		return;
	}

	micro = ioreq->io_BrkTime ? ioreq->io_BrkTime : 20000;
	unit->BreakTimer->Request.io_Command = TR_ADDREQUEST;
	unit->BreakTimer->Time.Seconds = micro / 1000000;
	unit->BreakTimer->Time.Microseconds = micro % 1000000;
	unit->BreakTimerPending = TRUE;
	unit->BreakActive = TRUE;

	SendIO( (PTR)unit->BreakTimer );

	if ( ioreq->io_SerFlags & SERF_QUEUEDBRK )
	{
		unit->BreakRequest = ioreq;
	}
	else
	{
		Serial_Reply( ctx, ioreq );
	}
}

// --

SEC_CODE void
Serial_Cmd_Break( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	if ( ( unit->ACM ) && ( ! ( unit->ACMCapabilities & SERIAL_ACM_BREAK ) ) )
	{
		ioreq->IOSer.io_Error = IOERR_NOCMD;

		Serial_Reply( ctx, ioreq );

		return;
	}

	if ( ! unit->BreakTimerOpen )
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;

		Serial_Reply( ctx, ioreq );

		return;
	}

	if ( ioreq->io_SerFlags & SERF_QUEUEDBRK )
	{
		AddTail( &unit->WriteList, (PTR)ioreq );

		Serial_Write_Start( ctx, unit );
	}
	else
	{
		Serial_BreakStart( ctx, unit, ioreq );
	}
}

// --
