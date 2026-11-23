
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE S32
USB2_Master_QueueCommand( struct USBBase *usbbase, struct CmdHeader *command )
{
	struct USB2_MsgPort *port;
	struct ExecIFace *IExec;

	S32 queued;

	IExec = usbbase->usb_IExec;
	queued = FALSE;

	IExec->Disable();

	port = usbbase->usb_Master_MsgPort;

	if ( ( port ) && ( ! usbbase->usb_Master_Task.tn_Parent ) &&
		( command->cm_Task != usbbase->usb_Master_Task.tn_TaskAdr ) )
	{
		MSGPORT_PUTMSG( port, command );

		queued = TRUE;
	}

	IExec->Enable();

	return ( queued );
}

// --
