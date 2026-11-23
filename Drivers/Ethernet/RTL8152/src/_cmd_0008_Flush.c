
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

void
Cmd_Flush( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	ioreq->ios2_WireError = S2WERR_GENERIC_ERROR;

	// Flush queued reads and writes belonging to this opener.
	if ( ioreq->ios2_BufferManagement )
    {
		AbortAll( unit, (APTR) ioreq->ios2_BufferManagement, S2WERR_GENERIC_ERROR );

		ioreq->ios2_Req.io_Error = S2ERR_NO_ERROR;
	}
	else
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
	}

	ReplySanaRequest( unit, (PTR) ioreq );

}

// --
