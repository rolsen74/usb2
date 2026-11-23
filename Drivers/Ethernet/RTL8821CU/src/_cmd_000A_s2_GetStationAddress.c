
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
	static const U8 empty[SANA2_MAX_ADDR_BYTES] = { 0 };

	CopyMem( (PTR) empty, ioreq->ios2_SrcAddr, sizeof( empty ) );

	CopyMem( (PTR) empty, ioreq->ios2_DstAddr, sizeof( empty ) );

	if ( ( ! unit->unit_AddressValid ) || ( unit->unit_Detached ) )
	{
		ReportUnitOffline( unit, ioreq );

		return;
	}

	CopyMem( unit->unit_StationAddress, ioreq->ios2_SrcAddr, RTL8821CU_ADDR_SIZE );

	CopyMem( unit->unit_StationAddress, ioreq->ios2_DstAddr, RTL8821CU_ADDR_SIZE );
}

// --
