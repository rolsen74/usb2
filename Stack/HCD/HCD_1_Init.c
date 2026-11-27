
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

#include "HCD_1_Init_1_Misc.c"
#include "HCD_1_Init_2_Add_Int.c"
#include "HCD_1_Init_3_Add_HUB.c"

// --

SEC_CODE static S32
__myInit ( struct USBBase * usbbase, PTR userdata, PTR in_ptr )
{
	struct USB2_HCDNode * hn;
	struct intern *		  in;
	S32					  retval;
	S32					  undo;
	const char *		  stage;

	retval = FALSE;
	undo   = FALSE;
	hn	   = userdata;
	in	   = in_ptr;
	stage  = "validate controller";

	USBDEBUG ( "__myInit (HCD)           : Enter : HN %p", hn );
	TASK_NAME_ENTER ( "HCD : __myInit" );

#ifdef DO_DEBUG
	if ( in )
	{
		in->StructID = ID_IN_HCD;
	}
#endif

	// --

	if ( HCD_VALID ( hn ) != VSTAT_Okay )
	{
		USBDEBUG ( "__myInit (HCD)           : Invalid HCD (%p)", hn );
		goto bailout;
	}

	// --
	// We are running in Task Init, so tn_TaskAdr have not been set
	// so we set it too avoid loosing Interrupt Signals
	hn->hn_Task->tn_TaskAdr = TASK_FIND ();
	undo					= TRUE;

	// --

	USBDEBUG ( "__myInit (HCD)           : 1" );

	stage = "set up messages and timer";

	if ( ! __Init_1_Misc ( usbbase, hn, in ) )
	{
		USBDEBUG ( "__myInit (HCD)           : __Init_1_Misc() Failed" );
		goto bailout;
	}

	USBDEBUG ( "__myInit (HCD)           : 2 : Adr %p", hn->HCD_Functions.Chip_Preinit );

	if ( hn->HCD_Functions.Chip_Preinit )
	{
		stage = "read controller capabilities";

		if ( ! hn->HCD_Functions.Chip_Preinit ( hn ) )
		{
			USBDEBUG ( "__myInit (HCD)           : Chip_Preinit() Failed" );
			goto bailout;
		}
	}

	USBDEBUG ( "__myInit (HCD)           : 3 : Adr %p", hn->HCD_Functions.Chip_Reset );

	if ( hn->HCD_Functions.Chip_Reset )
	{
		stage = "reset controller";

		if ( ! hn->HCD_Functions.Chip_Reset ( hn ) )
		{
			USBDEBUG ( "__myInit (HCD)           : Chip_Reset() Failed" );
			goto bailout;
		}
	}

	USBDEBUG ( "__myInit (HCD)           : 4 : Adr %p", hn->HCD_Functions.Chip_Alloc );

	if ( hn->HCD_Functions.Chip_Alloc )
	{
		stage = "allocate controller resources";

		if ( ! hn->HCD_Functions.Chip_Alloc ( hn ) )
		{
			USBDEBUG ( "__myInit (HCD)           : Chip_Alloc() Failed" );
			goto bailout;
		}
	}

	USBDEBUG ( "__myInit (HCD)           : 5" );

	stage = "install interrupt and reset handlers";

	if ( ! __Init_1_Add_Int ( usbbase, hn ) )
	{
		USBDEBUG ( "__myInit (HCD)           : __Init_1_Add_Int() Failed" );
		goto bailout;
	}

	USBDEBUG ( "__myInit (HCD)           : 6" );

	if ( hn->HCD_Functions.Chip_Init )
	{
		stage = "initialize controller";

		if ( ! hn->HCD_Functions.Chip_Init ( hn ) )
		{
			USBDEBUG ( "__myInit (HCD)           : Chip_Init() Failed" );
			goto bailout;
		}
	}

	USBDEBUG ( "__myInit (HCD)           : 7" );

	if ( hn->HCD_Functions.Chip_Start )
	{
		stage			 = "start controller";
		hn->hn_DMAActive = TRUE;

		if ( ! hn->HCD_Functions.Chip_Start ( hn ) )
		{
			USBDEBUG ( "__myInit (HCD)           : Chip_Start() Failed" );
			goto bailout;
		}
	}

	USBDEBUG ( "__myInit (HCD)           : 8" );

	stage = "allocate root hub";

	if ( ! __Init_1_Add_HUB ( usbbase, hn, in ) )
	{
		USBDEBUG ( "__myInit (HCD)           : __Init_1_Add_HUB() Failed" );
		goto bailout;
	}

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	hn->hn_AcceptIO = TRUE;

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	USBDEBUG ( "__myInit (HCD)           : 9" );

	// -- Start Timer Tick
	in->Tick_TimerAdded = TRUE;
	IO_SEND ( &in->Tick_TimeRequest );

	retval = TRUE;

bailout:

	if ( ! retval )
	{
		usbbase->usb_IExec->DebugPrintF ( "USB : HCD startup could not %s\n", stage );
	}

	if ( ( ! retval ) && ( undo ) )
	{
		// Undo setting of TaskAdr
		hn->hn_Task->tn_TaskAdr = NULL;
	}

	// -- Exit

	USBDEBUG ( "__myInit (HCD)           : Leave (retval %ld)", retval );

	TASK_NAME_LEAVE ();

	return ( retval );
}

// --
