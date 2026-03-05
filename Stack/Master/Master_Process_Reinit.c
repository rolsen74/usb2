
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
// We need to reassign MsgPorts and Signals
// Never return FALSE if you changed some MsgPorts/Signals

SEC_CODE static S32 __myProcess_Reinit( struct USBBase *usbbase, struct intern *in )
{
struct USB2_TaskNode *tn;
struct ExecIFace *IExec;
S32 retval;

	retval = FALSE;

	// --

	IExec = usbbase->usb_IExec;
	IExec->Disable();

	tn = & usbbase->usb_Master_Task;
	tn->tn_TaskAdr = TASK_FIND();

	TASK_REALLOCSIGNAL( & in->Task_Shutdown_Signal );

	MSGPORT_REINIT( & in->Task_Shutdown_MsgPort );

	MSGPORT_REINIT( & in->Cmd_MsgPort );

	MSGPORT_REINIT( & in->Tick_MsgPort );
	
	IExec->Enable();

	// --

	retval = TRUE;

// bailout:

	return( retval );
}

// --
