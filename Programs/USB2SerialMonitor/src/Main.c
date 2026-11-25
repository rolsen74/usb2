
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Monitor.h"

#include <stdio.h>
#include <string.h>

// --

struct IntuitionIFace *IIntuition;

static const char Version[] __attribute__((used)) = "$VER: USB2SerialMonitor 1.1 (25.11.2026)";

static const char * const ClassNames[CLASS_COUNT] =
{
	"window.class",
	"layout.gadget",
	"button.gadget",
	"integer.gadget",
	"chooser.gadget",
	"label.image",
	"texteditor.gadget",
	"scroller.gadget",
	"string.gadget"
};

// --

static void
ScheduleRefresh( struct TimeRequest *timer )
{
	timer->Request.io_Command = TR_ADDREQUEST;
	timer->Time.Seconds = 0;
	timer->Time.Microseconds = 100000;

	SendIO( (PTR) timer );
}

// --

static void
RunMonitor( struct Monitor *monitor )
{
	U32 serialSignal;
	U32 timerSignal;
	U32 signals;

	serialSignal = 1UL << monitor->SerialPort->mp_SigBit;
	timerSignal = 1UL << monitor->TimerPort->mp_SigBit;
	monitor->Run = TRUE;
	monitor->Dirty = TRUE;

	Capture_Refresh( monitor );

	ScheduleRefresh( monitor->Timer );

	while( monitor->Run )
	{
		signals = Wait( Monitor_Signals( monitor ) | serialSignal | timerSignal | SIGBREAKF_CTRL_C );

		if ( signals & SIGBREAKF_CTRL_C )
		{
			break;
		}

		if ( signals & serialSignal )
		{
			Capture_Handle( monitor );
		}

		if ( signals & Monitor_Signals( monitor ) )
		{
			Monitor_HandleWindows( monitor );
		}

		if ( ( monitor->Run ) && ( signals & timerSignal ) && ( GetMsg( monitor->TimerPort ) ) )
		{
			Capture_Refresh( monitor );

			ScheduleRefresh( monitor->Timer );
		}
	}

	if ( ! CheckIO( (PTR) monitor->Timer ) )
	{
		AbortIO( (PTR) monitor->Timer );
	}

	WaitIO( (PTR) monitor->Timer );
}

// --

int
main( void )
{
	struct Library *intuition;
	struct Monitor *monitor;

	U32 index;
	int result;
	BOOL timerOpen;

	result = RETURN_FAIL;
	timerOpen = FALSE;
	intuition = NULL;

	monitor = AllocVecTags( sizeof( *monitor ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! monitor )
	{
		return ( RETURN_FAIL );
	}

	intuition = OpenLibrary( "intuition.library", 50 );

	if ( ! intuition )
	{
		goto bailout;
	}

	IIntuition = (PTR) GetInterface( intuition, "main", 1, NULL );

	if ( ! IIntuition )
	{
		goto bailout;
	}

	for( index = 0; index < CLASS_COUNT; index++ )
	{
		monitor->Bases[index] = OpenClass( ClassNames[index], 50, & monitor->Classes[index] );

		if ( ! monitor->Bases[index] )
		{
			fprintf( stderr, "USB2SerialMonitor could not open %s.\n", ClassNames[index] );

			goto bailout;
		}
	}

	monitor->SerialPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	monitor->TimerPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	monitor->Data = AllocVecTags( CAPTURE_LIMIT, TAG_END );

	if ( ( ! monitor->SerialPort ) || ( ! monitor->TimerPort ) || ( ! monitor->Data ) )
	{
		goto bailout;
	}

	monitor->Serial = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *monitor->Serial ),
		ASOIOR_ReplyPort, monitor->SerialPort, TAG_END );

	monitor->Timer = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *monitor->Timer ),
		ASOIOR_ReplyPort, monitor->TimerPort, TAG_END );

	if ( ( ! monitor->Serial ) || ( ! monitor->Timer ) )
	{
		goto bailout;
	}

	if ( OpenDevice( TIMERNAME, UNIT_MICROHZ, (PTR) monitor->Timer, 0 ) )
	{
		goto bailout;
	}

	timerOpen = TRUE;
	monitor->Settings.Baud = 115200;
	monitor->Settings.DataBits = 8;
	monitor->Settings.StopBits = 1;

	strcpy( monitor->SavePath, "RAM:USB2SerialCapture.bin" );

	if ( ! Monitor_OpenMain( monitor ) )
	{
		goto bailout;
	}

	RunMonitor( monitor );

	result = RETURN_OK;

bailout:
	Capture_Stop( monitor );

	if ( IIntuition )
	{
		Monitor_CloseDialog( monitor );

		DisposeObject( monitor->Main );

		for( index = CLASS_COUNT; index > 0; index-- )
		{
			if ( monitor->Bases[index - 1] )
			{
				CloseClass( monitor->Bases[index - 1] );
			}
		}

		DropInterface( (PTR) IIntuition );
	}

	if ( intuition )
	{
		CloseLibrary( intuition );
	}

	if ( timerOpen )
	{
		CloseDevice( (PTR) monitor->Timer );
	}

	FreeSysObject( ASOT_IOREQUEST, monitor->Timer );

	FreeSysObject( ASOT_IOREQUEST, monitor->Serial );

	FreeSysObject( ASOT_PORT, monitor->TimerPort );

	FreeSysObject( ASOT_PORT, monitor->SerialPort );

	FreeVec( monitor->Data );

	FreeVec( monitor );

	if ( result != RETURN_OK )
	{
		fprintf( stderr, "USB2SerialMonitor could not start.\n" );
	}

	return ( result );
}

// --
