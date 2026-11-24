
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

// Called with usb_HID_Semaphore held. Report only the first press and final release.
SEC_CODE U32
HID_Hold_Input( struct USBBase *usbbase, U32 slot, U32 qualifier, U32 down )
{
	U32 *held;

	held = &usbbase->usb_HID_Held[slot];

	if ( down )
	{
		if ( ++*held == 1 )
		{
			usbbase->usb_HID_Qualifiers |= qualifier;

			return ( TRUE );
		}
	}
	else if ( ( *held ) && ( --*held == 0 ) )
	{
		usbbase->usb_HID_Qualifiers &= ~qualifier;

		return ( TRUE );
	}

	return ( FALSE );
}

// --
