
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

SEC_CODE void __Task_Signal( struct USBBase *usbbase, PTR task, U32 signals )
{
struct ExecIFace *IExec;

//	USBDEBUG( "ooo, task %p, signals %08lx", task, signals );

	#if 0
	#ifdef DO_DEBUG
	IExec = usbbase->usb_IExec;
	IExec->DebugPrintF( "%p: --> Signal ( %p , $%08lx )\n", IExec->FindTask(NULL), task, signals );
	#endif
	#endif

	if (( task ) && ( signals ))
	{
		IExec = usbbase->usb_IExec;
		IExec->Signal( task, signals );
	}
}

// --
