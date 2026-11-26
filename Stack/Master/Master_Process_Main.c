
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

SEC_CODE void
__myProcess_Main ( struct USBBase * usbbase, struct intern * in )
{
	U32 mask;
	U32 wait;

	while ( TRUE )
	{
		if ( in->Stopping )
		{
			myHandle_Cmds ( usbbase, in );

			if ( ( ! in->LauncherRunning ) && ( in->Shutdown_TM_List.uh_Nodes == 0 )
				 && ( usbbase->usb_HCDASync.ua_Counter == 0 ) )
			{
				break;
			}
		}

		/*
		** Build Wait Mask
		*/

		// Master Exit signal
		wait = SIGBREAKF_CTRL_C;

		// Master Refresh signal
		wait |= SIGBREAKF_CTRL_D;

		// Fullboot signal
		wait |= SIGBREAKF_CTRL_E;

		// a Subtask dident stop in time .. timeout, crashed?
		wait |= in->Task_Shutdown_MsgPort.ump_Signal.sig_Signal_Mask;

		// External User Command
		wait |= in->Cmd_MsgPort.ump_Signal.sig_Signal_Mask;

		// Tick Timer
		wait |= in->Tick_MsgPort.ump_Signal.sig_Signal_Mask;

		// Notify Reply
		wait |= in->Notify_ReplyMsgPort.ump_Signal.sig_Signal_Mask;

		//		usbbase->usb_IExec->DebugPrintF( "__myProcess_Main : Task %p : Wait $%08lx\n", TASK_FIND(), wait );

		mask = TASK_WAIT ( wait );

		//		usbbase->usb_IExec->DebugPrintF( "__myProcess_Main : Task %p : Mask $%08lx\n", TASK_FIND(), mask );

		if ( usbbase->usb_Master_Task.tn_Parent )
		{
			in->Stopping = TRUE;
		}

		if ( mask & SIGBREAKF_CTRL_D )
		{
			// Just a refresh, so we can break the Wait()
			USBDEBUG ( "__myProcess_Main (Master)        : Got : CTRL+D" );
			//			USBERROR( "__myProcess_Main (Master)        : Got : CTRL+D" );
			myHandle_Task_Ended ( usbbase, in );

			myFree_DNs ( usbbase );
			myFree_TNs ( usbbase );
		}

		if ( mask & in->Tick_MsgPort.ump_Signal.sig_Signal_Mask )
		{
			USBDEBUG ( "__myProcess_Main (Master)        : Got : Tick Timer" );
			if ( MSGPORT_GETMSG ( &in->Tick_MsgPort ) )
			{
				myFree_DNs ( usbbase );
				myFree_TNs ( usbbase );
				in->Tick_TimeRequest.Time.Seconds	   = 2;
				in->Tick_TimeRequest.Time.Microseconds = 0;
				IO_SEND ( &in->Tick_TimeRequest );
			}
		}

		// a Subtask dident stop in time .. timeout, crashed?
		if ( mask & in->Task_Shutdown_MsgPort.ump_Signal.sig_Signal_Mask )
		{
			USBDEBUG ( "__myProcess_Main (Master)        : Got : Task_Shutdown_MsgPort" );
			//			USBERROR( "__myProcess_Main (Master)        : Got : Task_Shutdown_MsgPort" );
			myHandle_Task_Timeout ( usbbase, in );
		}

		// External User Command
		if ( mask & in->Cmd_MsgPort.ump_Signal.sig_Signal_Mask )
		{
			USBDEBUG ( "__myProcess_Main (Master)        : Got : Cmd_MsgPort" );
			//			USBERROR( "__myProcess_Main (Master)        : Got : Cmd_MsgPort" );
			myHandle_Cmds ( usbbase, in );
		}

		// Notify Reply
		if ( mask & in->Notify_ReplyMsgPort.ump_Signal.sig_Signal_Mask )
		{
			USBDEBUG ( "__myProcess_Main (Master)        : Got : Notify_Reply" );

			while ( TRUE )
			{
				PTR msg = MSGPORT_GETMSG ( &in->Notify_ReplyMsgPort );

				if ( ! msg )
				{
					break;
				}

				USB2_Notify_Free ( usbbase, msg );
			}
		}
		if ( ( mask & SIGBREAKF_CTRL_E ) && ( ! in->Stopping ) && ( ! in->LauncherStarted ) )
		{
			in->LauncherStarted = TRUE;

			__myProcess_LaunchDrivers ( usbbase, in );
		}

	}
}

// --
