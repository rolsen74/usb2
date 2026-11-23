
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE void
__Task_Stop_TN ( struct USBBase * usbbase, struct USB2_TaskNode * tn, STR file UNUSED )

#else

SEC_CODE void
__Task_Stop_TN ( struct USBBase * usbbase, struct USB2_TaskNode * tn )

#endif

{
	struct USB2_TaskMsg * tm;
	struct Task *		  task;

	if ( ( ! tn ) || ( TASK_LOCK ( tn ) != LSTAT_Okay ) )
	{
		return;
	}

	tm = TASKMSG_ALLOC ();

	if ( ( tm ) && ( usbbase->usb_Master_MsgPort ) )
	{
		tm->tm_Cmd.cm_Command = CMD_STOP_PROCESS;
		tm->tm_Task			  = tn;

		// The master owns the message and its task reference after PutMsg.
		MSGPORT_PUTMSG ( usbbase->usb_Master_MsgPort, tm );

		return;
	}

	if ( tm )
	{
		TASKMSG_FREE ( tm );
	}

	// A stop must still reach the worker if tracking could not be allocated.
	usbbase->usb_IExec->Disable ();

	task = tn->tn_TaskAdr;

	if ( task )
	{
		tn->tn_StopRequested = TRUE;

		usbbase->usb_IExec->Signal ( task, SIGBREAKF_CTRL_C );
	}

	usbbase->usb_IExec->Enable ();

	TASK_UNLOCK ( tn );
}

// --
