
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include <devices/timer.h>

// --

static struct MsgPort *TimerPort;
static struct TimeRequest *TimerRequest;
U32 WindowTimerMask;

// --

static void
Schedule_Window_Timer( void )
{
	TimerRequest->Request.io_Command = TR_ADDREQUEST;
	TimerRequest->Time.Seconds = 0;
	TimerRequest->Time.Microseconds = 500000;

	SendIO( &TimerRequest->Request );
}

// --

S32
Start_Window_Timer( void )
{
	if ( TimerRequest )
	{
		return ( TRUE );
	}

	TimerPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! TimerPort )
	{
		return ( FALSE );
	}

	TimerRequest = AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_ReplyPort, TimerPort, ASOIOR_Size, sizeof( *TimerRequest ), TAG_END );

	if ( ! TimerRequest )
	{
		goto failed;
	}

	if ( OpenDevice( TIMERNAME, UNIT_MICROHZ, &TimerRequest->Request, 0 ) )
	{
		FreeSysObject( ASOT_IOREQUEST, TimerRequest );

		TimerRequest = NULL;

		goto failed;
	}

	WindowTimerMask = 1UL << TimerPort->mp_SigBit;

	Schedule_Window_Timer();

	return ( TRUE );

failed:

	FreeSysObject( ASOT_PORT, TimerPort );

	TimerPort = NULL;

	return ( FALSE );
}

// --

void
Stop_Window_Timer( void )
{
	if ( ! TimerRequest )
	{
		return;
	}

	if ( ! CheckIO( &TimerRequest->Request ) )
	{
		AbortIO( &TimerRequest->Request );
	}

	WaitIO( &TimerRequest->Request );

	CloseDevice( &TimerRequest->Request );

	FreeSysObject( ASOT_IOREQUEST, TimerRequest );

	FreeSysObject( ASOT_PORT, TimerPort );

	TimerRequest = NULL;
	TimerPort = NULL;
	WindowTimerMask = 0;
}

// --

void
Handle_Window_Timer( void )
{
	if ( ( TimerPort ) && ( GetMsg( TimerPort ) ) )
	{
		Update_Window_Timer();

		Schedule_Window_Timer();
	}
}

// --
