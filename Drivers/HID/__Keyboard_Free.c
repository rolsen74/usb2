
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

SEC_CODE void __Keyboard_Free( struct USBBase *usbbase UNUSED, PTR userdata UNUSED, PTR in UNUSED )
{
struct HID_ReportNode *rn;
//struct HIDData *hd;

	TASK_NAME_ENTER( "__Keyboard_Free" );

	rn = userdata;
//	hd = rn->HIDData;

	// --

	if ( rn->Timer_Added )
	{
		IO_STOP( & rn->Timer_IOReq );
		rn->Timer_Added = FALSE;
	}

	MSGPORT_FREE( & rn->Timer_MsgPort );
	MSGPORT_FREE( & rn->Input_MsgPort );
	MSGPORT_FREE( & rn->IOReq_MsgPort );

	// --

	TASK_NAME_LEAVE();
}

// --
