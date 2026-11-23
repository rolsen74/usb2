
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void Unit_Free( struct EthernetUnit *unit )
{
	Sana2_FreeList( & unit->unit_MulticastRanges );

	Sana2_FreeList( & unit->unit_TrackedTypes );

	Sana2_FreeList( & unit->unit_Multicast );

	FreeVec( unit );
}

// --
