
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Commands.h"
#include "Radio.h"

// --

void
Cmd_Online( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	if ( ! unit->unit_Configured )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
		ioreq->ios2_WireError = S2WERR_NOT_CONFIGURED;
	}
	else if ( ( ! unit->unit_TxReady ) || ( ! unit->unit_AssocID ) || ( RTL8188EU_DeferredLink( unit ) && ! unit->unit_Operational ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
		ioreq->ios2_WireError = S2WERR_UNIT_DISCONNECTED;
	}
	else if ( ! unit->unit_Online )
	{
		unit->unit_Online = TRUE;

		RTL8188EU_SubmitRX( unit );

		SignalSana2Event( unit, S2EVENT_ONLINE );
	}

	USBINFO( "RTL8188EU: S2_ONLINE error %ld, wire error %lu, configured %lu, TX %lu, AID %lu, online %lu",
		(S32) ioreq->ios2_Req.io_Error, ioreq->ios2_WireError, (U32) unit->unit_Configured,
		(U32) unit->unit_TxReady, (U32) unit->unit_AssocID, (U32) unit->unit_Online );
}

// --
