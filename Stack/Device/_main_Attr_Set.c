
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static U32
_main_Attr_Set( struct USB2IFace *Self, USB2_ID id UNUSED, U32 tag, PTR buffer, U32 buffersize )
{
	struct USBBase *usbbase;

	usbbase = (PTR) Self->Data.LibBase;

	if ( ( tag == USB2Tag_Attr_IOCaptureFilter ) && ( buffer ) && ( buffersize == sizeof( struct USB2_IOCaptureFilter ) ) )
	{
		if ( USB2_Notify_SetIOFilter( usbbase, buffer ) )
		{
			return ( buffersize );
		}
	}

	return ( 0 );
}

// --
