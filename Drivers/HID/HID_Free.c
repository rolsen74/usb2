
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

SEC_CODE void HID_Free( struct USBBase *usbbase, struct HIDData *hd )
{
struct HID_ReportNode *rn;

	USBDEBUG( "HID : HID_Free" );
	TASK_NAME_ENTER( "HID_Free" );

	// --

	// A failed worker may still have a task-node reference held by Master.
	ASYNC_WAIT( &hd->ASync_Drivers );

	if ( hd->Register )
	{
		USBDEBUG( "HID_Free              : Unregister" );
		REGISTER_UNREGISTER( hd->Register );
		hd->Register = NULL;
	}

	// --

	MSGPORT_FREE( & hd->Timer_MsgPort );
	ASYNC_FREE( & hd->ASync_Drivers );

	while( TRUE )
	{
		rn = NODE_REMHEAD( & hd->Reports );

		if ( ! rn )
		{
			break;
		}

		Node_Report_Free( usbbase, rn );
	}

	if ( hd->ReportBuffer )
	{
		MEM_FREEVEC( hd->ReportBuffer );
		hd->ReportBuffer = NULL;
	}

	// --

	TASK_NAME_LEAVE();
}

// --
