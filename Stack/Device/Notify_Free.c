
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE void
USB2_Notify_Free( struct USBBase *usbbase, struct USB2_NotifyMessage *msg )
{
	if ( msg->nm_Type == USBNT_IORequest )
	{
		SEMAPHORE_OBTAIN( &usbbase->usb_Notify_Semaphore );

		usbbase->usb_Notify_IOPending--;

		SEMAPHORE_RELEASE( &usbbase->usb_Notify_Semaphore );
	}

	MEMORY_FREE( MEMID_NotifyMessage, msg, 0 );
}

// --
