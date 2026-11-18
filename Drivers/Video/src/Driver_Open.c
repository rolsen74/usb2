/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE PTR Driver_Open( struct LibraryManagerInterface *Self, U32 version UNUSED )
{
struct VideoDriverBase *driver;

	driver = (PTR) Self->Data.LibBase;
	driver->Library.lib_OpenCnt++;
	driver->Library.lib_Flags &= ~LIBF_DELEXP;

	return ( driver );
}

// --
