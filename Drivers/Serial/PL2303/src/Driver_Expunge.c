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
	// The registered serial device calls into this library after the USB task exits.
	return ( NULL );
}

// --
