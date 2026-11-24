
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static S32
Open_Timer( struct PrinterUnit *unit, struct PrinterTimer *timer )
{
	timer->Request = AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( struct TimeRequest ), ASOIOR_ReplyPort, unit->unit_CloseTimerPort, TAG_END );

	if ( ! timer->Request )
	{
		return ( FALSE );
	}

	timer->Open = ( OpenDevice( "timer.device", UNIT_MICROHZ, (PTR)timer->Request, 0 ) == 0 );

	return ( timer->Open );
}

// --

S32
Printer_CloseTimerInit( struct PrinterUnit *unit )
{
	unit->unit_CloseTimerPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ( ! unit->unit_CloseTimerPort ) || ( ! Open_Timer( unit, &unit->unit_CloseTimer ) ) ||
		( ! Open_Timer( unit, &unit->unit_IdleTimer ) ) )
	{
		return ( FALSE );
	}

	if ( unit->unit_Dot4_Active )
	{
		unit->unit_Dot4_Clock = (PTR)GetInterface( (PTR)unit->unit_CloseTimer.Request->Request.io_Device, "main", 1, NULL );

		return ( unit->unit_Dot4_Clock != NULL );
	}

	return ( TRUE );
}

// --

void
Printer_TimerStop( struct PrinterTimer *timer )
{
	if ( timer->Pending )
	{
		if ( ! CheckIO( (PTR)timer->Request ) )
		{
			AbortIO( (PTR)timer->Request );
		}

		WaitIO( (PTR)timer->Request );

		timer->Pending = FALSE;
	}
}

// --

static void
Free_Timer( struct PrinterTimer *timer )
{
	Printer_TimerStop( timer );

	if ( timer->Open )
	{
		CloseDevice( (PTR)timer->Request );
	}

	if ( timer->Request )
	{
		FreeSysObject( ASOT_IOREQUEST, timer->Request );
	}
}

// --

void
Printer_CloseTimerFree( struct PrinterUnit *unit )
{
	if ( unit->unit_Dot4_Clock )
	{
		DropInterface( (PTR)unit->unit_Dot4_Clock );
	}

	Free_Timer( &unit->unit_IdleTimer );

	Free_Timer( &unit->unit_CloseTimer );

	if ( unit->unit_CloseTimerPort )
	{
		FreeSysObject( ASOT_PORT, unit->unit_CloseTimerPort );
	}
}

// --

void
Printer_TimerArm( struct PrinterTimer *timer, U32 usec )
{
	timer->Request->Request.io_Command = TR_ADDREQUEST;
	timer->Request->Time.Seconds = usec / 1000000;
	timer->Request->Time.Microseconds = usec % 1000000;
	timer->Pending = TRUE;

	SendIO( (PTR)timer->Request );
}

// --
