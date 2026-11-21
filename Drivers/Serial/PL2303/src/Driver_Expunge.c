/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE PTR Driver_Expunge( struct LibraryManagerInterface *Self UNUSED )
{
	// Keep the external USB driver resident across hotplug.
	return ( NULL );
}

// --
