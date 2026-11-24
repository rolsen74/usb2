
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
	U32 index;

	if ( ! unit->unit_AddressValid )
	{
		ReportUnitOffline( unit, ioreq );

		return;
	}

	if ( unit->unit_Configured )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
		ioreq->ios2_WireError = S2WERR_IS_CONFIGURED;

		if ( ( ! unit->unit_AssocID ) && ( ! ( ioreq->ios2_Req.io_Flags & USB2WIRELESS_IOF_MANAGER_CONFIG ) ) )
		{
			unit->unit_StartAuto = TRUE;
		}

		return;
	}

	for( index = 0; index < RTL8821CU_ADDR_SIZE; index++ )
	{
		if ( ioreq->ios2_SrcAddr[index] != unit->unit_StationAddress[index] )
		{
			ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
			ioreq->ios2_WireError = S2WERR_SRC_ADDRESS;

			return;
		}
	}

	unit->unit_Configured = TRUE;

	if ( ! ( ioreq->ios2_Req.io_Flags & USB2WIRELESS_IOF_MANAGER_CONFIG ) )
	{
		unit->unit_StartAuto = TRUE;
	}
}

// --
