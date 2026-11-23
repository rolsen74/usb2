
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct EthernetUnit *
Unit_Alloc( void )
{
	struct EthernetUnit *unit;

	unit = AllocVecTags( sizeof( *unit ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! unit )
	{
		return ( NULL );
	}

	InitSemaphore( & unit->unit_CommandLock );

	InitSemaphore( & unit->unit_WorkLock );

	NewList( & unit->unit_Pending );

	NewList( & unit->unit_MulticastRanges );

	NewList( & unit->unit_TrackedTypes );

	return ( unit );
}

// --
