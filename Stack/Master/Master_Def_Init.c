
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

SEC_CODE static S32
__myTask_Def_Init( struct USBBase *usbbase, struct intern *in )
{
	S32 retval;

	retval = FALSE;

	USBINFO( "__myTask_Def_Init (Master)" );

	if ( ! MSGPORT_INIT( & in->Task_Shutdown_MsgPort ))
	{
		USBERROR( "__myTask_Def_Init (Master) : Shutdown port error" );
		goto bailout;
	}

	// --

	if ( ! MSGPORT_INIT( & in->Cmd_MsgPort ))
	{
		USBERROR( "__myTask_Def_Init (Master) : Command port error" );
		goto bailout;
	}

	// --

	if ( ! MSGPORT_INIT( & in->Notify_ReplyMsgPort ))
	{
		USBERROR( "__myTask_Def_Init (Master) : Notify port error" );
		goto bailout;
	}

	usbbase->usb_Notify_ReplyMsgPort = & in->Notify_ReplyMsgPort;

	// -- Timer Tick

	if ( ! MSGPORT_INIT( & in->Tick_MsgPort ))
	{
		USBERROR( "__myTask_Def_Init (Master) : Tick port error" );
		goto bailout;
	}

	MEM_COPY( usbbase->usb_TimeRequest, & in->Tick_TimeRequest, sizeof( struct TimeRequest ));

	in->Tick_TimeRequest.Request.io_Message.mn_ReplyPort = & in->Tick_MsgPort.ump_MsgPort;
	in->Tick_TimeRequest.Request.io_Command = TR_ADDREQUEST;
	in->Tick_TimeRequest.Time.Microseconds = 0;
	in->Tick_TimeRequest.Time.Seconds = 2;

	in->Tick_TimerAdded = TRUE;
	IO_SEND( & in->Tick_TimeRequest );

	usbbase->usb_Master_MsgPort = & in->Cmd_MsgPort;

	retval = TRUE;

bailout:

	return ( retval );
}

// --
