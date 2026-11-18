
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

void
RTL8188EU_DropLink( struct EthernetUnit *unit )
{
	unit->unit_AssocID = 0;
	unit->unit_Operational = FALSE;

	RTL8188EU_ClearKeys( unit );

	if ( unit->unit_Online )
	{
		unit->unit_Online = FALSE;

		RTL8188EU_AbortData( unit, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );

		SignalSana2Event( unit, S2EVENT_OFFLINE );
	}
}

// --
