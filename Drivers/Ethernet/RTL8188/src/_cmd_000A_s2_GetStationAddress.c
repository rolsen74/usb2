
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
Cmd_GetStationAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	if ( ! unit->unit_StationAddressValid )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		return;
	}

	memset( ioreq->ios2_SrcAddr, 0, sizeof( ioreq->ios2_SrcAddr ) );

	memset( ioreq->ios2_DstAddr, 0, sizeof( ioreq->ios2_DstAddr ) );

	memcpy( ioreq->ios2_SrcAddr, unit->unit_StationAddress, sizeof( unit->unit_StationAddress ) );

	memcpy( ioreq->ios2_DstAddr, unit->unit_StationAddress, sizeof( unit->unit_StationAddress ) );
}

// --
