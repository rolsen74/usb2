
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
Cmd_Invalid( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	ioreq->ios2_Req.io_Error = IOERR_NOCMD;

	ReplySanaRequest( unit, (PTR) ioreq );

}

// --
