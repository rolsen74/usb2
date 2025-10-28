
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

SEC_CODE void __Misc_ColdReboot( struct USBBase *usbbase )
{
	USBDEBUG( "Cold Reboot" );

	usbbase->usb_IExec->ColdReboot();
}

// --
