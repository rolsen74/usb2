
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
RTL8821CU_CheckLink( struct EthernetUnit *unit )
{
	if ( ( ! unit->unit_RadioReady ) || ( ! unit->unit_AssocID ) ||
		( ++unit->unit_BeaconAge < unit->unit_BeaconLimit ) )
	{
		return;
	}

	DebugPrintF( "RTL8821CU: associated access point beacon timeout\n" );

	RTL8821CU_DropLink( unit );
}

// --
