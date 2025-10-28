
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

SEC_CODE void __Semaphore_Release( struct USBBase *usbbase UNUSED, struct USB2_Semaphore *us )
{
struct ExecIFace *IExec;

	#ifdef DO_PANIC

	if ( ! us )
	{
		USBPANIC( "Semaphore_Release : 1 : NULL Pointer" );
	}

	if ( us->us_StructID != ID_USB2_SEMA )
	{
		USBPANIC( "Semaphore_Release : 2 : Invalid ID" );
	}

	#endif

	IExec = usbbase->usb_IExec;
	IExec->ReleaseSemaphore( & us->us_Semaphore );
}

// --
