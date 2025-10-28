
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

SEC_CODE void __Mem_FreeVec( struct USBBase *usbbase, PTR mem )
{
struct ExecIFace *IExec;

	if ( mem )
	{
		IExec = usbbase->usb_IExec;
		IExec->FreeVec( mem );
	}
}

// --
