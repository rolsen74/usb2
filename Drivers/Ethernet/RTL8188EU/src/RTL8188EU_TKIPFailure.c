
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
RTL8188EU_TKIPFailure( struct EthernetUnit *unit )
{
	U64 now;

	now = RTL8188EU_TKIPTime();

	if ( ( unit->unit_TKIPLastFailure ) && ( now - unit->unit_TKIPLastFailure < 60000000 ) )
	{
		unit->unit_TKIPBlockedUntil = now + 60000000;
		unit->unit_TKIPLastFailure = 0;

		RTL8188EU_DropLink( unit );
	}
	else
	{
		unit->unit_TKIPLastFailure = now;
	}

	Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_RX );
}

// --
