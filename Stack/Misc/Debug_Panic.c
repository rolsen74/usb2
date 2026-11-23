
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --
#ifdef DO_PANIC

SEC_CODE void VARARGS68K
__Debug_Panic( struct USBBase *usbbase, STR filename, U32 linenr, STR fmt, ... )
{
	struct ExecIFace *IExec;

	STR buf;
	va_list ap;

	buf = NULL;

	IExec = USB2_GetExecIFace();

	IExec->DebugPrintF( "USBPANIC!!\n" );

	if ( ( usbbase ) && ( usbbase->usb_IUtility ) )
	{
		va_start( ap, fmt );

		buf = usbbase->usb_IUtility->VASPrintf( fmt, va_getlinearva( ap, PTR ) );

		va_end( ap );
	}

	IExec->Disable();

	IExec->DebugPrintF( "\n\n-- USBPANIC --\n" );

	IExec->DebugPrintF( "%s:%04ld:%s\n", filename, linenr, ( buf ) ? buf : fmt );

	IExec->DebugPrintF( "-- USBPANIC --\n\n\n" );

	IExec->Wait( 0 );
}

#endif

// --
