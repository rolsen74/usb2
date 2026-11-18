/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE PTR Driver_Close( struct LibraryManagerInterface *Self )
{
struct AudioDriverBase *driver;

	driver = (PTR) Self->Data.LibBase;

	if ( driver->Library.lib_OpenCnt )
	{
		driver->Library.lib_OpenCnt--;
	}

	return ( NULL );
}

// --
