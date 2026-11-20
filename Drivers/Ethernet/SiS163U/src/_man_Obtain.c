
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Manager.h"

// --

U32
_manager_Obtain( struct DeviceManagerInterface *Self )
{
	USBDEBUG( "SIS163U : _manager_Obtain" );

	Self->Data.RefCount++;

	return ( Self->Data.RefCount );
}

// --
