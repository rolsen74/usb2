
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static U32 _manager_Release( struct DeviceManagerInterface *Self )
{
	USBDEBUG( "Printer : _manager_Release" );

	Self->Data.RefCount--;

	return( Self->Data.RefCount );
}

// --
