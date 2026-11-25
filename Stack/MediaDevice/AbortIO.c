
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "MediaDevice.h"

// --

SEC_CODE void
MediaDevice_AbortIO( struct DeviceManagerInterface *Self, struct IORequest *request )
{
	struct USB2MediaDevice *base;
	struct DeviceManagerInterface *backend;

	base = (PTR) Self->Data.LibBase;

	if ( request->io_Unit )
	{
		backend = MediaDevice_Backend( base );

		backend->AbortIO( request );
	}
}

// --
