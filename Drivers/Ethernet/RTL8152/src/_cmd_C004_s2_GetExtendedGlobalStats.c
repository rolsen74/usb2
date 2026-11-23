
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
	struct Sana2ExtDeviceStats stats;

	ioreq->ios2_WireError = 0;
	stats = (struct Sana2ExtDeviceStats) { 0 };
	stats.s2xds_PacketsReceived.s2q_High = unit->unit_PacketsReceived >> 32;
	stats.s2xds_PacketsReceived.s2q_Low = unit->unit_PacketsReceived;
	stats.s2xds_PacketsSent.s2q_High = unit->unit_PacketsSent >> 32;
	stats.s2xds_PacketsSent.s2q_Low = unit->unit_PacketsSent;
	stats.s2xds_BadData.s2q_High = unit->unit_BadData >> 32;
	stats.s2xds_BadData.s2q_Low = unit->unit_BadData;
	stats.s2xds_Overruns.s2q_High = unit->unit_Overruns >> 32;
	stats.s2xds_Overruns.s2q_Low = unit->unit_Overruns;
	stats.s2xds_UnknownTypesReceived.s2q_High = unit->unit_UnknownTypesReceived >> 32;
	stats.s2xds_UnknownTypesReceived.s2q_Low = unit->unit_UnknownTypesReceived;
	stats.s2xds_Reconfigurations.s2q_High = unit->unit_Reconfigurations >> 32;
	stats.s2xds_Reconfigurations.s2q_Low = unit->unit_Reconfigurations;
	stats.s2xds_LastStart = unit->unit_LastStart;

	// The Ethernet driver does not maintain session timestamps.
	Sana2_GetExtendedGlobalStats( ioreq, & stats, offsetof( struct Sana2ExtDeviceStats, s2xds_LastConnected ) );

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
