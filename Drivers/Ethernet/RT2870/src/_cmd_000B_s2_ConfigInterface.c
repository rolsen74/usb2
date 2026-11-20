
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "WiFiCommand.h"

// --

void
Cmd_ConfigInterface( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	if ( unit->unit_Configured )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
		ioreq->ios2_WireError = S2WERR_IS_CONFIGURED;
	}
	else if ( memcmp( ioreq->ios2_SrcAddr, unit->unit_StationAddress, sizeof( unit->unit_StationAddress ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_SRC_ADDRESS;
	}
	else
	{
		unit->unit_Configured = TRUE;
	}

	if ( ( ! unit->unit_AssocID ) &&
		( ! ( ioreq->ios2_Req.io_Flags & USB2WIRELESS_IOF_MANAGER_CONFIG ) ) &&
		( ( ! ioreq->ios2_Req.io_Error ) || ( ioreq->ios2_WireError == S2WERR_IS_CONFIGURED ) ) )
	{
		unit->unit_WiFiStartPending = TRUE;

		Signal( unit->unit_TaskAdr, SIGBREAKF_CTRL_E );
	}

	USBINFO( "RT2870: S2_CONFIGINTERFACE error %ld, wire error %lu",
		(S32) ioreq->ios2_Req.io_Error, ioreq->ios2_WireError );
}

// --
