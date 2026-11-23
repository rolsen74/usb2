
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "Master.h"

// --

static void
__myProcess_Entry ( void )
{
	struct MasterStartMsg * tm;
	struct Task *			parent;
	struct intern *			in;
	struct ExecIFace *		IExec;
	struct USBBase *		usbbase;

	// --

	TASK_NAME_ENTER ( "__myProcess_LaunchDrivers" );

	IExec	= USB2_GetExecIFace ();
	tm		= (PTR)( IExec->FindTask ( NULL )->tc_UserData );
	usbbase = tm->usbbase;
	parent	= tm->Parent;
	in		= usbbase->usb_Master_Intern;

	Master_ReplyStart( IExec, tm, TRUE );

	// Master must be free to service driver start commands during the scan.
	if ( __myProcess_ScanDriverDir( usbbase ) )
	{
		SEMAPHORE_OBTAIN ( &usbbase->usb_FDriver_Semaphore );

		usbbase->usb_ClassesReady = TRUE;

		SEMAPHORE_RELEASE ( &usbbase->usb_FDriver_Semaphore );

		USB2_Bind_Enumerated ( usbbase );

		USB2_FullBoot_Complete( usbbase, TRUE );
	}
	else
	{
		USB2_FullBoot_Complete( usbbase, FALSE );
	}

	IExec->Disable ();

	in->LauncherRunning = FALSE;

	IExec->Signal ( parent, SIGBREAKF_CTRL_D );

	TASK_NAME_LEAVE ();

	IExec->Enable ();
}

// --

SEC_CODE static void
__myProcess_LaunchDrivers ( struct USBBase * usbbase, struct intern * in )
{
	struct MasterStartMsg msg;
	struct USB2_Signal	  sig;
	struct DOSIFace *	  IDOS;

	sig.sig_StructID = 0;

	if ( TASK_ALLOCSIGNAL ( &sig ) )
	{
		IDOS = usbbase->usb_IDOS;

		msg.usbbase = usbbase;
		msg.Parent	= TASK_FIND ();
		msg.Result	= MASTER_START_PENDING;
		msg.Mask	= sig.sig_Signal_Mask;

		in->LauncherRunning = TRUE;

		if ( IDOS->CreateNewProcTags ( NP_Name, "Universal Serial Bus - Launcher", NP_Child, FALSE, NP_Priority, 1, NP_StackSize,
									   1024 * 32, NP_Entry, __myProcess_Entry, NP_UserData, &msg, TAG_END ) )
		{
			do
			{
				TASK_WAIT( msg.Mask );
			}
			while ( msg.Result == MASTER_START_PENDING );
		}
		else
		{
			in->LauncherRunning = FALSE;

			USBERROR ( "__myProcess_LaunchDrivers : Process error" );

			USB2_FullBoot_Complete ( usbbase, FALSE );
		}

		TASK_FREESIGNAL ( &sig );
	}
	else
	{
		USBERROR ( "__myProcess_LaunchDrivers : Signal error" );

		USB2_FullBoot_Complete ( usbbase, FALSE );
	}
}

// --
