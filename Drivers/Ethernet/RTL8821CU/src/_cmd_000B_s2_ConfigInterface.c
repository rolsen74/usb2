
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
Cmd_ConfigInterface( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	if ( ! ValidStationAddress( ioreq->ios2_SrcAddr ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_SRC_ADDRESS;

		return;
	}

	ReportUnitOffline( unit, ioreq );
}

// --
