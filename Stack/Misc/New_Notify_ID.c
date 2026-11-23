
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE USB2_ID
__Misc_NewNotifyID ( struct USBBase * usbbase )
{
	USB2_ID id;

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	do
	{
		id = ++usbbase->usb_Notify_ID;
	} while ( id < 200 );

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	return ( id );
}

// --
