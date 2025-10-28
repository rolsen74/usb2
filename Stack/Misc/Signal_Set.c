
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

SEC_CODE U32 __Task_SetSignal( struct USBBase *usbbase, U32 a, U32 b )
{
struct ExecIFace *IExec;
U32 mask;

	IExec = usbbase->usb_IExec;
	mask = IExec->SetSignal( a, b );

	return( mask );
}

// --
