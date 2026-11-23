
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
__Misc_Wait( struct USBBase *usbbase, U32 micro )
{
	struct USB2_MsgPort mp;
	struct TimeRequest req;
	struct ExecIFace *IExec;

	S32 own_signal;
	S32 error;
	U32 pending;

	IExec = usbbase->usb_IExec;
	pending = 0;

	MEM_SET( &mp, 0, sizeof( mp ) );

	own_signal = MSGPORT_INIT( &mp );

	if ( ! own_signal )
	{
		// DoIO waits for the timer reply even if Ctrl-F is already pending.
		pending = IExec->SetSignal( 0, 0 ) & SIGBREAKF_CTRL_F;

		IExec->NewList( &mp.ump_MsgPort.mp_MsgList );

		mp.ump_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
		mp.ump_MsgPort.mp_Flags = PA_SIGNAL;
		mp.ump_MsgPort.mp_SigBit = SIGBREAKB_CTRL_F;

		mp.ump_MsgPort.mp_SigTask = TASK_FIND();
	}

	MEM_COPY( usbbase->usb_TimeRequest, &req, sizeof( req ) );

	req.Request.io_Message.mn_ReplyPort = &mp.ump_MsgPort;
	req.Request.io_Command = TR_ADDREQUEST;
	req.Time.Seconds = micro / 1000000;
	req.Time.Microseconds = micro % 1000000;

	error = IO_DO( &req );

	if ( own_signal )
	{
		MSGPORT_FREE( &mp );
	}
	else if ( pending )
	{
		IExec->SetSignal( pending, pending );
	}

	return ( error );
}

// --
