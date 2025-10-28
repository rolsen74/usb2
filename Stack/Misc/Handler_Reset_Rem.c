
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

SEC_CODE void __Misc_RemResetCallback( struct USBBase *usbbase, struct Interrupt *i )
{
struct ExecIFace *IExec;

	USBINFO( "Removing Reset Callback %p", i );

	IExec = usbbase->usb_IExec;

	IExec->RemResetCallback( i );
}

// --
