
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
MediaDevice_BeginIO( struct DeviceManagerInterface *Self, struct IORequest *request )
{
	struct USB2MediaDevice *base;
	struct DeviceManagerInterface *backend;

	base = (PTR) Self->Data.LibBase;

	if ( request->io_Command == NSCMD_DEVICEQUERY )
	{
		USB2_DeviceQuery( (PTR) request, NSDEVTYPE_UNKNOWN, base->Commands );
	}
	else if ( request->io_Message.mn_Length < sizeof( struct IOStdReq ) )
	{
		request->io_Error = IOERR_BADLENGTH;
	}
	else if ( request->io_Unit )
	{
		backend = MediaDevice_Backend( base );

		backend->BeginIO( request );

		return;
	}
	else
	{
		((struct IOStdReq *) request)->io_Actual = 0;
		request->io_Error = IOERR_OPENFAIL;
	}

	if ( ! ( request->io_Flags & IOF_QUICK ) )
	{
		base->IExec->ReplyMsg( (PTR) request );
	}
}

// --
