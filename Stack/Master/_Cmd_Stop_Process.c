
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static void
__Cmd_Stop_Process ( struct USBBase * usbbase, struct intern * in, struct USB2_TaskMsg * tm )
{
	struct USB2_TaskNode * tn;
	struct Task *		   task;

	tn = tm->tm_Task;

	usbbase->usb_IExec->Disable ();

	task = tn->tn_TaskAdr;

	if ( ( ! task ) || ( tn->tn_StopRequested ) )
	{
		usbbase->usb_IExec->Enable ();

		TASKMSG_FREE ( tm );

		return;
	}

	tn->tn_StopRequested = TRUE;

	usbbase->usb_IExec->Signal ( task, SIGBREAKF_CTRL_C );

	usbbase->usb_IExec->Enable ();

	MEM_COPY ( usbbase->usb_TimeRequest, &tm->tm_TimeRequest, sizeof ( struct TimeRequest ) );

	tm->tm_TimeReq_Active								  = TRUE;
	tm->tm_TimeRequest.Request.io_Message.mn_Node.ln_Name = (PTR)tm;
	tm->tm_TimeRequest.Request.io_Message.mn_ReplyPort	  = &in->Task_Shutdown_MsgPort.ump_MsgPort;
	tm->tm_TimeRequest.Request.io_Command				  = TR_ADDREQUEST;
	tm->tm_TimeRequest.Time.Seconds						  = 30;
	tm->tm_TimeRequest.Time.Microseconds				  = 0;

	NODE_ADDTAIL ( &in->Shutdown_TM_List, tm );

	IO_SEND ( &tm->tm_TimeRequest );
}

// --

SEC_CODE static void
myHandle_Task_Timeout ( struct USBBase * usbbase, struct intern * in )
{
	struct USB2_TaskMsg *  tm;
	struct USB2_TaskNode * tn;
	struct Message *	   msg;

	while ( TRUE )
	{
		msg = MSGPORT_GETMSG ( &in->Task_Shutdown_MsgPort );

		if ( ! msg )
		{
			break;
		}

		tm					  = (PTR)msg->mn_Node.ln_Name;
		tm->tm_TimeReq_Active = FALSE;
		tn					  = tm->tm_Task;

		if ( ( ! tn ) || ( tn->tn_State == TASK_State_Stopped ) )
		{
			RetireStopMessage ( tm, usbbase, in );

			continue;
		}

		if ( ! tm->tm_StopReported )
		{
			usbbase->usb_IExec->DebugPrintF ( "USB : '%s' overdue after 30 seconds (task type %lu, state %lu)\n",
											  tn->tn_Name[0] ? tn->tn_Name : "USB worker", (U32)tn->tn_Type, (U32)tn->tn_State );

			tm->tm_StopReported = TRUE;
		}

		tm->tm_TimeReq_Active				 = TRUE;
		tm->tm_TimeRequest.Time.Seconds		 = 5;
		tm->tm_TimeRequest.Time.Microseconds = 0;
		IO_SEND ( &tm->tm_TimeRequest );
	}
}

// --
