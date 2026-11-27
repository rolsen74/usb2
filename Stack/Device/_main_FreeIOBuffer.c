
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static void
_main_FreeIOBuffer( struct USB2IFace *Self, PTR buffer )
{
	struct USBBase *usbbase;

	usbbase = (PTR) Self->Data.LibBase;

	Memory_FreeIOBuffer( usbbase, buffer );
}

// --
