
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE USB2_ID __Misc_NewNotifyID( struct USBBase *usbbase )
{
USB2_ID id;

	SEMAPHORE_OBTAIN( & usbbase->usb_NotifySemaphore );

	do
	{
		id = ++usbbase->usb_NotifyID;
	} 
	while( ! id );
	
	SEMAPHORE_RELEASE( & usbbase->usb_NotifySemaphore );

	return( id );
}

// --
