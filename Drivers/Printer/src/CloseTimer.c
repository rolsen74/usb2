
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Printer_CloseTimerInit( struct PrinterUnit *unit )
{
	unit->unit_CloseTimerPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! unit->unit_CloseTimerPort )
	{
		return ( FALSE );
	}

	unit->unit_CloseTimer = AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( struct TimeRequest ), ASOIOR_ReplyPort, unit->unit_CloseTimerPort, TAG_END );

	if ( ! unit->unit_CloseTimer )
	{
		return ( FALSE );
	}

	unit->unit_CloseTimerOpen = ( OpenDevice( "timer.device", UNIT_MICROHZ, (PTR)unit->unit_CloseTimer, 0 ) == 0 );

	return ( unit->unit_CloseTimerOpen );
}

// --

void
Printer_CloseTimerStop( struct PrinterUnit *unit )
{
	if ( unit->unit_CloseTimerPending )
	{
		if ( ! CheckIO( (PTR)unit->unit_CloseTimer ) )
		{
			AbortIO( (PTR)unit->unit_CloseTimer );
		}

		WaitIO( (PTR)unit->unit_CloseTimer );

		unit->unit_CloseTimerPending = FALSE;
	}
}

// --

void
Printer_CloseTimerFree( struct PrinterUnit *unit )
{
	Printer_CloseTimerStop( unit );

	if ( unit->unit_CloseTimerOpen )
	{
		CloseDevice( (PTR)unit->unit_CloseTimer );
	}

	if ( unit->unit_CloseTimer )
	{
		FreeSysObject( ASOT_IOREQUEST, unit->unit_CloseTimer );
	}

	if ( unit->unit_CloseTimerPort )
	{
		FreeSysObject( ASOT_PORT, unit->unit_CloseTimerPort );
	}
}

// --

void
Printer_CloseTimerArm( struct PrinterUnit *unit, U32 seconds )
{
	unit->unit_CloseTimer->Request.io_Command = TR_ADDREQUEST;
	unit->unit_CloseTimer->Time.Seconds = seconds;
	unit->unit_CloseTimer->Time.Microseconds = 0;
	unit->unit_CloseTimerPending = TRUE;

	SendIO( (PTR)unit->unit_CloseTimer );
}

// --
