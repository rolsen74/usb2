
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
s2_GetGlobalStats( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	struct Sana2DeviceStats stats;

	ioreq->ios2_WireError = 0;
	stats = (struct Sana2DeviceStats) { 0 };
	stats.PacketsReceived		= unit->unit_PacketsReceived;
	stats.PacketsSent			= unit->unit_PacketsSent;
	stats.BadData				= unit->unit_BadData;
	stats.Overruns				= unit->unit_Overruns;
	stats.UnknownTypesReceived	= unit->unit_UnknownTypesReceived;
	stats.Reconfigurations		= unit->unit_Reconfigurations;
	stats.LastStart			= unit->unit_LastStart;

	Sana2_GetGlobalStats( ioreq, & stats );

	IExec->ReplyMsg( (APTR) ioreq );
}

// --
