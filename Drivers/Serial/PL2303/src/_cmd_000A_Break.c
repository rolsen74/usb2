
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

SEC_CODE static void
ScheduleBreak( struct SerialUnit *unit, U32 micro )
{
	unit->BreakTimer->Request.io_Command = TR_ADDREQUEST;
	unit->BreakTimer->Time.Seconds = micro / 1000000;
	unit->BreakTimer->Time.Microseconds = micro % 1000000;
	unit->BreakTimerPending = TRUE;

	SendIO( (PTR)unit->BreakTimer );
}

// --

SEC_CODE static void
FinishBreakRequest( struct SerialContext *ctx, struct SerialUnit *unit, S32 error )
{
	struct IOExtSer *ioreq;

	ioreq = unit->BreakRequest;
	unit->BreakRequest = NULL;

	if ( ioreq )
	{
		ioreq->IOSer.io_Error = error;

		Serial_Reply( ctx, ioreq );
	}
}

// --

SEC_CODE void
Serial_BreakStop( struct SerialContext *ctx, struct SerialUnit *unit, S32 error )
{
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

	ifcnr = unit->ACM ? unit->StartMsg->InterfaceDescriptor->InterfaceNumber : 0;

	if ( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x23, 0, ifcnr, NULL, 0 ) )
	{
		if ( ( ! unit->Detached ) && ( unit->BreakRetries < 3 ) )
		{
			unit->BreakRetries++;
			unit->BreakFinite = FALSE;

			ScheduleBreak( unit, 100000 );

			if ( ! error )
			{
				return;
			}
		}
		else
		{
			unit->BreakFailed = TRUE;
		}

		FinishBreakRequest( ctx, unit, error ? error : SerErr_DevBusy );

		return;
	}

	unit->BreakActive = FALSE;
	unit->BreakFailed = FALSE;
	unit->BreakFinite = FALSE;

	FinishBreakRequest( ctx, unit, error );
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

		if ( ( unit->BreakFinite ) && ( ! unit->BreakTimer->Request.io_Error ) )
		{
			unit->BreakActive = FALSE;
			unit->BreakFinite = FALSE;

			FinishBreakRequest( ctx, unit, 0 );
		}
		else
		{
			Serial_BreakStop( ctx, unit, unit->BreakTimer->Request.io_Error ? SerErr_DevBusy : 0 );
		}

		Serial_Write_Start( ctx, unit );
	}
}

// --

SEC_CODE void
Serial_BreakStart( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	U32 micro;
	U16 ifcnr;
	U16 duration;

	ifcnr = unit->ACM ? unit->StartMsg->InterfaceDescriptor->InterfaceNumber : 0;

	micro = ioreq->io_BrkTime ? ioreq->io_BrkTime : 20000;
	duration = 0xffff;

	if ( ( unit->ACM ) && ( micro < 65535000 ) && ( ! ( micro % 1000 ) ) )
	{
		duration = micro / 1000;
	}

	if ( ( unit->BreakActive ) || ( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x23, duration, ifcnr, NULL, 0 ) ) )
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;

		Serial_Reply( ctx, ioreq );

		return;
	}

	unit->BreakActive = TRUE;
	unit->BreakFinite = ( duration != 0xffff );
	unit->BreakRetries = 0;
	unit->BreakFailed = FALSE;

	ScheduleBreak( unit, micro );

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
