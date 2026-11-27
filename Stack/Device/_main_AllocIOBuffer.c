
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static PTR
_main_AllocIOBuffer( struct USB2IFace *Self, U32 size )
{
	struct USBBase *usbbase;

	usbbase = (PTR) Self->Data.LibBase;

	return ( Memory_AllocIOBuffer( usbbase, size ) );
}

// --
