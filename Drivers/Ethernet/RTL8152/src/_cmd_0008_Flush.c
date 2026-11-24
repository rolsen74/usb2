
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
	AbortAll( unit, NULL, S2WERR_GENERIC_ERROR );

	ioreq->ios2_WireError = 0;
	ioreq->ios2_Req.io_Error = S2ERR_NO_ERROR;

	ReplySanaRequest( unit, (PTR) ioreq );
}

// --
