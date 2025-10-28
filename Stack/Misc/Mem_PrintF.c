
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

SEC_CODE STR VARARGS68K __Mem_PrintF( struct USBBase *usbbase, STR fmt, ... )
{
struct UtilityIFace *IUtility;
va_list ap;
STR str;

	#ifdef DO_DEBUG

	if (( ! usbbase ) || ( ! fmt ))
	{
		USBPANIC( "Error: Mem_PrintF( %p, %p )", usbbase, fmt );
	}

	#endif

	if ( fmt )
	{
		va_start( ap, fmt );

		IUtility = usbbase->usb_IUtility;
		str = IUtility->VASPrintf( fmt, va_getlinearva( ap, STR ));
//		str = NULL;

		va_end( ap );
	}
	else
	{
		str = NULL;
	}

	#ifdef DO_DEBUG
	if ( str )
	{
		USBDEBUG( "--> '%s'", str );
	}
	#endif

	return( str );
}

// --
