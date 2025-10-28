
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

SEC_CODE void __Misc_RemIntServer( struct USBBase *usbbase, U32 nr, struct Interrupt *i )
{
struct ExecIFace *IExec;

	USBINFO( "Removing Interrupt %ld Server %p", nr, i );

	IExec = usbbase->usb_IExec;

	IExec->RemIntServer( nr, i );
}

// --
