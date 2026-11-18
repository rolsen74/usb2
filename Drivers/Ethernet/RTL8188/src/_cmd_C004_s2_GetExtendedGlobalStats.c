
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
Cmd_GetExtendedGlobalStats( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct Sana2ExtDeviceStats *stats;

	U32 length;
	U32 offset;

	stats = ioreq->ios2_StatData;
	offset = offsetof( struct Sana2ExtDeviceStats, s2xds_PacketsReceived );

	if ( ! stats )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	if ( stats->s2xds_Length < offset )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
		ioreq->ios2_WireError = S2WERR_BAD_STATDATA;

		return;
	}

	length = stats->s2xds_Length < sizeof( *stats ) ? stats->s2xds_Length : sizeof( *stats );

	memset( (U8 *) stats + offset, 0, length - offset );

	if ( length >= offsetof( struct Sana2ExtDeviceStats, s2xds_PacketsSent ) )
	{
		stats->s2xds_PacketsReceived.s2q_High = unit->unit_PacketsRx >> 32;
		stats->s2xds_PacketsReceived.s2q_Low = unit->unit_PacketsRx;
	}

	if ( length >= offsetof( struct Sana2ExtDeviceStats, s2xds_BadData ) )
	{
		stats->s2xds_PacketsSent.s2q_High = unit->unit_PacketsTx >> 32;
		stats->s2xds_PacketsSent.s2q_Low = unit->unit_PacketsTx;
	}

	if ( length >= offsetof( struct Sana2ExtDeviceStats, s2xds_Reconfigurations ) )
	{
		stats->s2xds_UnknownTypesReceived.s2q_High = unit->unit_UnknownPackets >> 32;
		stats->s2xds_UnknownTypesReceived.s2q_Low = unit->unit_UnknownPackets;
	}

	stats->s2xds_Actual = length;
}

// --
