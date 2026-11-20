
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
Cmd_Online( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
	ioreq->ios2_WireError = unit->unit_Configured ? S2WERR_UNIT_DISCONNECTED : S2WERR_NOT_CONFIGURED;

	USBINFO( "SiS163U: S2_ONLINE failed (error %ld, wire error %lu), association support pending",
		(S32) ioreq->ios2_Req.io_Error, ioreq->ios2_WireError );
}

// --
