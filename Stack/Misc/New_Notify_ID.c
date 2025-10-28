
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
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
