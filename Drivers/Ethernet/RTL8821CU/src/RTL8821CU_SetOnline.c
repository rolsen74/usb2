
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

void
RTL8821CU_SetOnline( struct EthernetUnit *unit, S32 online )
{
	if ( unit->unit_Online == online )
	{
		return;
	}

	unit->unit_Online = online;

	if ( online )
	{
		unit->unit_PacketsRx = 0;
		unit->unit_PacketsTx = 0;
		unit->unit_UnknownPackets = 0;
		unit->unit_BadPackets = 0;

		Sana2_GetTime( & unit->unit_LastStart );
	}

	DebugPrintF( "RTL8821CU: unit %lu %s\n", unit->unit_UnitNr, online ? "online" : "offline" );

	Sana2_SignalEvent( & unit->unit_Pending, online ? S2EVENT_ONLINE : S2EVENT_OFFLINE );
}

// --
