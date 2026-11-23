
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
ReportUnitOffline( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
	ioreq->ios2_WireError = unit->unit_Detached ? S2WERR_UNIT_DISCONNECTED : S2WERR_UNIT_OFFLINE;
}

// --
