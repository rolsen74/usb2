
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
	USBDEBUG( "HID : HID_Free" );
	TASK_NAME_ENTER( "HID_Free" );

	// --

	if ( hd->Register )
	{
		USBDEBUG( "HID_Free              : Unregister" );
		REGISTER_UNREGISTER( hd->Register );
		hd->Register = NULL;
	}

	// --

	MSGPORT_FREE( & hd->Timer_MsgPort );
	ASYNC_FREE( & hd->ASync_Drivers );

	// --

	TASK_NAME_LEAVE();
}

// --
