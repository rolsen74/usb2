
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_CODE S32 __Boot_Mouse_Init( struct USBBase *usbbase, PTR userdata, PTR in UNUSED )
{
struct HID_ReportNode *rn;
// struct HIDData *hd;
S32 retval;

	TASK_NAME_ENTER( "__Boot_Mouse_Init" );

	rn = userdata;
//	hd = rn->HIDData;

	retval = FALSE;

	rn->TaskAdr = TASK_FIND();

	// --

	if ( ! MSGPORT_INIT( & rn->IOReq_MsgPort ))
	{
		USBDEBUG( "__Boot_Keyboard_Init : Command MsgPort init failed" );
		goto bailout;		
	}

	// --

	if ( ! MSGPORT_INIT( & rn->Input_MsgPort ))
	{
		USBDEBUG( "HID_Init : Error : Input MsgPort init failed" );
		goto bailout;		
	}

	MEM_COPY( usbbase->usb_InputIORequest, & rn->Input_IOReq, sizeof( struct IOStdReq ));
	rn->Input_IOReq.io_Message.mn_ReplyPort = & rn->Input_MsgPort.ump_MsgPort;

	// --

	retval = TRUE;

bailout:

	TASK_NAME_LEAVE();

	return( retval );
}

// --
