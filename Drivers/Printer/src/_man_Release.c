
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
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
