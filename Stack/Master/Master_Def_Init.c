
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"
#include "Master.h"

// --

SEC_CODE static S32 __myTask_Def_Init( struct USBBase *usbbase, struct intern *in )
{
S32 retval;

	retval = FALSE;
	USBINFO( "__myTask_Def_Init (Master)" );

	// 
	if ( ! TASK_ALLOCSIGNAL( & in->Task_Shutdown_Signal ))
	{
		USBERROR( "__myTask_Def_Init (Master) : 1 : Error" );
		goto bailout;
	}

	if ( ! MSGPORT_INIT( & in->Task_Shutdown_MsgPort ))
	{
		USBERROR( "__myTask_Def_Init (Master) : 2 : Error" );
		goto bailout;
	}

	if ( ! MSGPORT_INIT( & in->Cmd_MsgPort ))
	{
		USBERROR( "__myTask_Def_Init (Master) : 3 : Error" );
		goto bailout;
	}

	usbbase->usb_Master_MsgPort = & in->Cmd_MsgPort;

	// -- Timer Tick

	if ( ! MSGPORT_INIT( & in->Tick_MsgPort ))
	{
		USBERROR( "__myTask_Def_Init (Master) : 4 : Error" );
		goto bailout;
	}

	MEM_COPY( usbbase->usb_TimeRequest, & in->Tick_TimeRequest, sizeof( struct TimeRequest ));

	in->Tick_TimeRequest.Request.io_Message.mn_ReplyPort = & in->Tick_MsgPort.ump_MsgPort;
	in->Tick_TimeRequest.Request.io_Command = TR_ADDREQUEST;
	in->Tick_TimeRequest.Time.Microseconds = 0;
	in->Tick_TimeRequest.Time.Seconds = 2;

	in->Tick_TimerAdded = TRUE;
	IO_SEND( & in->Tick_TimeRequest );

	// --

	retval = TRUE;

bailout:

	return( retval );
}

// --
