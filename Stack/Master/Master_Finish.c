
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

SEC_CODE static void
Master_Finish( struct USBBase *usbbase )
{
	struct USB2_TaskNode *tn;
	struct ExecIFace *IExec;

	tn = &usbbase->usb_Master_Task;
	IExec = usbbase->usb_IExec;

	// Finish the task before the shutdown caller can release shared state.
	IExec->Disable();

	tn->tn_State = TASK_State_Stopped;
	tn->tn_TaskAdr = NULL;

	if ( tn->tn_Parent )
	{
		IExec->Signal( tn->tn_Parent, tn->tn_Mask );
	}
}

// --
