/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE U32 Driver_Release( struct LibraryManagerInterface *Self )
{
	Self->Data.RefCount--;

	return ( Self->Data.RefCount );
}

// --
