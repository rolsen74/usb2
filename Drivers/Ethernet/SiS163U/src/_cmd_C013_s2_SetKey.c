
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
Cmd_SetKey( struct EthernetUnit *unit UNUSED, struct IOSana2Req *ioreq )
{
	ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
	ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;
}

// --
