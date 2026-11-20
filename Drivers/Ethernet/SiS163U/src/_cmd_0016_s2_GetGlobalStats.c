
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
Cmd_GetGlobalStats( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct Sana2DeviceStats *stats;

	if ( ! ioreq->ios2_StatData )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	stats = ioreq->ios2_StatData;

	memset( stats, 0, sizeof( *stats ) );

	stats->PacketsReceived = unit->unit_PacketsRx;
	stats->PacketsSent = unit->unit_PacketsTx;
	stats->UnknownTypesReceived = unit->unit_UnknownPackets;
}

// --
