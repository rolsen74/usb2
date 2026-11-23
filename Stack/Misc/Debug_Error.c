
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "Debug_Print.h"

// --
#ifdef DO_ERROR

SEC_CODE void VARARGS68K
__Debug_Error( struct USBBase *usbbase, STR fmt, ... )
{
	va_list ap;

	va_start( ap, fmt );

	USB2_DebugPrint( usbbase, fmt, va_getlinearva( ap, PTR ) );

	va_end( ap );
}

#endif

// --
