
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

#include "Master.h"

// --

SEC_CODE static S32
Master_PrepareStart( struct ExecIFace *IExec, struct USBBase *usbbase, struct MasterStartMsg *msg )
{
	S32 signal;

	signal = IExec->AllocSignal( -1 );

	if ( signal >= 0 )
	{
		msg->usbbase = usbbase;

		msg->Parent = IExec->FindTask( NULL );

		msg->Result = MASTER_START_PENDING;
		msg->Mask = 1UL << signal;
	}

	return ( signal );
}

// --

SEC_CODE static S32
Master_WaitStart( struct ExecIFace *IExec, struct MasterStartMsg *msg, S32 signal, PTR task )
{
	S32 retval;

	retval = FALSE;

	if ( task )
	{
		do
		{
			IExec->Wait( msg->Mask );
		}
		while ( msg->Result == MASTER_START_PENDING );

		retval = msg->Result;
	}

	IExec->FreeSignal( signal );

	return ( retval );
}

// --

#include "Startup_Promote.c"
#include "Startup_Process.c"
#include "Startup_Task.c"

// --

SEC_CODE S32 __Master_Startup( struct USBBase *usbbase )
{
struct intern *in;
S32 retval;

	retval = FALSE;

//	usbbase->usb_IExec->DebugPrintF( "__Master_Startup 1\n" );

	// --

	in = MEM_ALLOCVEC( sizeof( struct intern ), TRUE );

	if ( ! in )
	{
		USBERROR( "__Master_Startup : Error Allocating memory" );
		goto bailout;
	}

	usbbase->usb_Master_Intern = in;

	#ifdef DO_DEBUG
	in->StructID = ID_IN_MASTER;
	#endif

	// --

	if (( ! __myMaster_StartProcess( usbbase )) && ( ! __myMaster_StartTask( usbbase )))
	{
		USBERROR( "__Master_Startup : Error Failed to start Master" );
		goto bailout;
	}

	#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

	if ( ! usbbase->usb_Master_Task.tn_TaskAdr )
	{
		USBPANIC( "__Master_Startup : Master Task NULL Pointer" );
	}

	if ( ! usbbase->usb_Master_MsgPort )
	{
		USBPANIC( "__Master_Startup : Master MsgPort NULL Pointer" );
	}

	#endif

	// --

	retval = TRUE;

bailout:

//	usbbase->usb_IExec->DebugPrintF( "__Master_Startup 99 : Retval %ld\n", retval );

	return( retval );
}

// --
