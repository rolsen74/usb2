
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static S32
PromoteHCD( struct USBBase *usbbase, struct USB2_HCDNode *hn, struct intern *in, U32 mask )
{
	struct USB2_WorkerHandoff handoff;

	MEM_SET( &handoff, 0, sizeof( handoff ) );

	handoff.Entry = __myMain;
	handoff.Data = hn;
	handoff.Ports[0] = &hn->hn_Abort_MsgPort.ump_MsgPort;
	handoff.Ports[1] = &hn->hn_Begin_MsgPort.ump_MsgPort;
	handoff.Ports[2] = &hn->hn_Delay_MsgPort.ump_MsgPort;
	handoff.Ports[3] = &hn->hn_WatchDog_MsgPort.ump_MsgPort;
	handoff.Ports[4] = &in->Tick_MsgPort.ump_MsgPort;
	handoff.Ports[5] = ( hn->hn_TTClearPort.ump_StructID == ID_USB2_MP ) ? &hn->hn_TTClearPort.ump_MsgPort : NULL;
	handoff.Owners[0] = &hn->hn_HUB_ASync.ua_Parent;
	handoff.Signals = hn->HCD_Mask | hn->HCD_Restart_Chip.sig_Signal_Mask | hn->hn_HUB_Signal.sig_Signal_Mask;
	handoff.Pending = mask;

	return ( USB2_PromoteWorker( usbbase, &handoff ) );
}

// --

SEC_CODE static S32
__Normal ( struct USBBase * usbbase, struct USB2_HCDNode * hn, struct intern * in )
{
	struct USB2_TaskNode * tn;
	U32					   mask;
	U32					   wait;

	tn = hn->hn_Task;

	USBDEBUG ( "__myMain (HCD)           : __Normal (1) :: HN %p : IN %p : TN %p :", hn, in, tn );
	TASK_NAME_ENTER ( "HCD Main : __Normal" );

	wait = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E | hn->HCD_Mask | hn->HCD_Restart_Chip.sig_Signal_Mask
		   | hn->hn_HUB_Signal.sig_Signal_Mask | hn->hn_Abort_MsgPort.ump_Signal.sig_Signal_Mask
		   | hn->hn_Begin_MsgPort.ump_Signal.sig_Signal_Mask | hn->hn_WatchDog_MsgPort.ump_Signal.sig_Signal_Mask
		   | in->Tick_MsgPort.ump_Signal.sig_Signal_Mask;

	while ( TRUE )
	{
		//		USBDEBUG( "__myMain (HCD)           : __Normal :: Index #%ld :: Mask $%08lx :: Tasks %ld :: HUB ASync %ld ::",
		//			hn->hn_HCDIndex, mask, usbbase->usb_Task_Header.uh_Nodes, hn->hn_HUB_ASync.ua_Counter );

		mask = TASK_WAIT ( wait );

		if ( ( mask & SIGBREAKF_CTRL_C ) && ( tn->tn_StopRequested ) )
		{
			break;
		}

		if ( ( mask & SIGBREAKF_CTRL_E ) && ( PromoteHCD( usbbase, hn, in, mask ) ) )
		{
			TASK_NAME_LEAVE();

			return ( TRUE );
		}

		//		usbbase->usb_IExec->DebugPrintF( "HCD Mask $%08lx\n", mask );

		// Lets put it first
		if ( mask & hn->HCD_Restart_Chip.sig_Signal_Mask )
		{
			if ( ! __myHandle_Reset ( usbbase, hn ) )
			{
				break;
			}

			mask &= ~hn->HCD_Mask;
		}

		if ( mask & hn->HCD_Mask )
		{
			//			usbbase->usb_IExec->DebugPrintF( " : __myHandle_HCD\n" );

			__myHandle_HCD ( usbbase, hn, mask );
		}

		if ( mask & hn->hn_WatchDog_MsgPort.ump_Signal.sig_Signal_Mask )
		{
			//			usbbase->usb_IExec->DebugPrintF( " : __myHandle_WatchDog\n" );

			__myHandle_WatchDog ( usbbase, hn );
		}

		if ( mask & hn->hn_Abort_MsgPort.ump_Signal.sig_Signal_Mask )
		{
			//			usbbase->usb_IExec->DebugPrintF( " : __myHandle_Abort\n" );

			__myHandle_Abort ( usbbase, hn );
		}

		if ( mask & hn->hn_Begin_MsgPort.ump_Signal.sig_Signal_Mask )
		{
			//			usbbase->usb_IExec->DebugPrintF( " : __myHandle_Begin\n" );

			__myHandle_Begin ( usbbase, hn );
		}

		if ( mask & hn->hn_HUB_Signal.sig_Signal_Mask )
		{
			//			usbbase->usb_IExec->DebugPrintF( " : ROOTHUB_HANDLE_CHANGE\n" );

			DrainDetachedFunctions ( usbbase, hn );

			if ( hn->hn_AcceptIO )
			{
				ROOTHUB_HANDLE_CHANGE ( hn );
			}
		}

		if ( mask & in->Tick_MsgPort.ump_Signal.sig_Signal_Mask )
		{
			//			usbbase->usb_IExec->DebugPrintF( " : sig_Signal_Mask\n" );

			if ( MSGPORT_GETMSG ( &in->Tick_MsgPort ) )
			{
				in->Tick_TimeRequest.Time.Seconds	   = 1;
				in->Tick_TimeRequest.Time.Microseconds = 0;
#ifdef DRV_UHCI

				if ( hn->hn_HCDType == HCDTYPE_UHCI )
				{
					in->Tick_TimeRequest.Time.Seconds = 0;
					in->Tick_TimeRequest.Time.Microseconds = UHCI_TICK_US;
				}

#endif

				IO_SEND ( &in->Tick_TimeRequest );
			}

			DrainDetachedFunctions ( usbbase, hn );

			__myHandle_HCD ( usbbase, hn, -1 );
		}

		if ( mask & SIGBREAKF_CTRL_D )
		{
			//			usbbase->usb_IExec->DebugPrintF( " : SIGBREAKF_CTRL_D" );

			USBDEBUG ( "__myMain (HCD)           : Got : CTRL+D" );
			// Used to break out of Wait so we can check ua_Counter in Stopping
		}

		//		usbbase->usb_IExec->DebugPrintF( "\n" );
	}

	//	usbbase->usb_IExec->DebugPrintF( "HCD Shutting down\n" );

	TASK_NAME_LEAVE ();

	return ( FALSE );
}

// --
