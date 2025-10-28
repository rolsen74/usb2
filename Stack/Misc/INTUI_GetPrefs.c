
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

SEC_CODE PTR __Misc_GetIntuiPrefs( struct USBBase *usbbase, PTR buf, U32 size )
{
struct IntuitionIFace *IIntuition;
PTR retval;

	USBDEBUG( "Get Intui Prefs( buf %p, size %ld )", buf, size );

	IIntuition = usbbase->usb_IIntuition;

	if ( ! IIntuition )
	{

	}

	if ( IIntuition )
	{
		retval = IIntuition->GetPrefs( buf, size );
	}
	else
	{
		USBDEBUG( "__Misc_GetIntuiPrefs : IIntuition : NULL Pointer" );
		retval = NULL;
	}

	return( retval );
}
