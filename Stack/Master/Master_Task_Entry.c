
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
// If DOS is not around, we have to run in a Task

SEC_CODE void
Master_Task_Entry( struct USBBase *usbbase, struct MasterStartMsg *msg )
{
	struct USB2_TaskNode *tn;
	struct ExecIFace *IExec;
	struct intern *in;

	IExec = usbbase->usb_IExec;
	in = usbbase->usb_Master_Intern;
	tn = &usbbase->usb_Master_Task;
	tn->tn_State = TASK_State_Starting;

	if ( ! __myTask_Init( usbbase, in ) )
	{
		__myTask_Free( usbbase, in );

		tn->tn_State = TASK_State_Stopped;

		Master_ReplyStart( IExec, msg, FALSE );

		return;
	}

	tn->tn_TaskAdr = TASK_FIND();

	tn->tn_State = TASK_State_Running;

	Master_ReplyStart( IExec, msg, TRUE );

	if ( __myTask_Main( usbbase, in ) == MAIN_RETURN_PROMOTE )
	{
		// Retire the old task before the replacement can dispatch work.
		IExec->Disable();

		in->PromotionReleased = TRUE;

		IExec->Signal( tn->tn_TaskAdr, SIGBREAKF_CTRL_E );

		return;
	}

	tn->tn_State = TASK_State_Stopping;

	__myTask_Free( usbbase, in );

	Master_Finish( usbbase );
}

// --
