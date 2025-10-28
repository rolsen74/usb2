
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

SEC_CODE void __Mem_Copy( struct USBBase *usbbase, PTR src, PTR dst, U32 size )
{
struct ExecIFace *IExec;

	#ifdef DO_PANIC

	if (( ! usbbase ) || ( ! src ) || ( ! dst ))
	{
		USBPANIC( "Mem_Copy : 1 : NULL Pointer" );
	}

	#endif

	if ( size > 0 )
	{
		IExec = usbbase->usb_IExec;
		IExec->CopyMem( src, dst, size );
	}
}

// --
