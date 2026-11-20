
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
RT2870_TKIPFailure( struct EthernetUnit *unit )
{
	U64 now;

	now = RT2870_TKIPTime();

	if ( ( unit->unit_TKIPLastFailure ) && ( now - unit->unit_TKIPLastFailure < 60000000 ) )
	{
		unit->unit_TKIPBlockedUntil = now + 60000000;
		unit->unit_TKIPLastFailure = 0;

		RT2870_DropLink( unit, FALSE );
	}
	else
	{
		unit->unit_TKIPLastFailure = now;
	}

	SignalSana2Event( unit, S2EVENT_ERROR | S2EVENT_RX );
}

// --
