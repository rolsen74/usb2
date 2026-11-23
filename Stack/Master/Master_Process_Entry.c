
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
// If DOS is around, we can start Process right away

SEC_CODE void
Master_Process_Entry( void )
{
	struct MasterStartMsg *msg;
	struct USB2_TaskNode *tn;
	struct ExecIFace *IExec;
	struct USBBase *usbbase;
	struct intern *in;

	IExec = USB2_GetExecIFace();

	msg = (PTR)( IExec->FindTask( NULL )->tc_UserData );

	usbbase = msg->usbbase;
	in = usbbase->usb_Master_Intern;
	tn = &usbbase->usb_Master_Task;
	tn->tn_State = TASK_State_Starting;

	if ( ! __myProcess_Init( usbbase, in ) )
	{
		__myProcess_Free( usbbase, in );

		tn->tn_State = TASK_State_Stopped;

		Master_ReplyStart( IExec, msg, FALSE );

		return;
	}

	tn->tn_TaskAdr = TASK_FIND();

	tn->tn_State = TASK_State_Running;

	Master_ReplyStart( IExec, msg, TRUE );

	__myProcess_Main( usbbase, in );

	tn->tn_State = TASK_State_Stopping;

	__myProcess_Free( usbbase, in );

	Master_Finish( usbbase );
}

// --
