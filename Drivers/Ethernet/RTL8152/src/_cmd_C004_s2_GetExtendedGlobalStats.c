
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
s2_GetExtendedGlobalStats( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	struct Sana2ExtDeviceStats *output;
	struct Sana2ExtDeviceStats stats;

	U32 available;

	output = ioreq->ios2_StatData;
	// The Ethernet driver does not maintain session timestamps.
	available = offsetof( struct Sana2ExtDeviceStats, s2xds_LastConnected );
	ioreq->ios2_WireError = 0;

	if ( ! output )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else if ( output->s2xds_Length < offsetof( struct Sana2ExtDeviceStats, s2xds_PacketsReceived ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
		ioreq->ios2_WireError = S2WERR_BAD_STATDATA;
	}
	else
	{
		memset( & stats, 0, sizeof( stats ) );

		stats.s2xds_Length = output->s2xds_Length;
		stats.s2xds_Actual = stats.s2xds_Length < available ? stats.s2xds_Length : available;
		stats.s2xds_PacketsReceived = unit->unit_PacketsReceived;
		stats.s2xds_PacketsSent = unit->unit_PacketsSent;
		stats.s2xds_BadData = unit->unit_BadData;
		stats.s2xds_Overruns = unit->unit_Overruns;
		stats.s2xds_UnknownTypesReceived = unit->unit_UnknownTypesReceived;
		stats.s2xds_Reconfigurations = unit->unit_Reconfigurations;
		stats.s2xds_LastStart = unit->unit_LastStart;

		memcpy( output, & stats, stats.s2xds_Actual );
	}

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
