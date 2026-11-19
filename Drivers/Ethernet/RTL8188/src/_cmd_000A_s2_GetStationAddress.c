
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
		USBERROR( "RTL8188EU: S2_GETSTATIONADDRESS failed, adapter has no valid station address" );

		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		return;
	}

	memset( ioreq->ios2_SrcAddr, 0, sizeof( ioreq->ios2_SrcAddr ) );

	memset( ioreq->ios2_DstAddr, 0, sizeof( ioreq->ios2_DstAddr ) );

	memcpy( ioreq->ios2_SrcAddr, unit->unit_StationAddress, sizeof( unit->unit_StationAddress ) );

	memcpy( ioreq->ios2_DstAddr, unit->unit_StationAddress, sizeof( unit->unit_StationAddress ) );

	USBINFO( "RTL8188EU: S2_GETSTATIONADDRESS %02lx:%02lx:%02lx:%02lx:%02lx:%02lx",
		(U32) unit->unit_StationAddress[0], (U32) unit->unit_StationAddress[1], (U32) unit->unit_StationAddress[2],
		(U32) unit->unit_StationAddress[3], (U32) unit->unit_StationAddress[4], (U32) unit->unit_StationAddress[5] );
}

// --
