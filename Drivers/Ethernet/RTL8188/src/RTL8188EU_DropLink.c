
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
	S32 online;
	S32 handshaking;

	online = unit->unit_Online;
	handshaking = RTL8188EU_DeferredLink( unit ) && unit->unit_AssocID;
	unit->unit_AssocID = 0;
	unit->unit_Operational = FALSE;
	unit->unit_Online = FALSE;
	unit->unit_SAE = FALSE;

	RTL8188EU_ClearKeys( unit );

	if ( ( online ) || ( handshaking ) )
	{
		RTL8188EU_AbortData( unit, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );
	}

	if ( online )
	{
		SignalSana2Event( unit, S2EVENT_OFFLINE );
	}
}

// --
