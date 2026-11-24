
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct EthernetUnit *Unit_Alloc( void )
{
struct EthernetUnit *unit;

	USBDEBUG( "RTL8188 : Unit_Alloc" );

	unit = AllocVecTags( sizeof( struct EthernetUnit ),
		AVT_ClearWithValue, 0,
		TAG_END
	);

	if ( ! unit )
	{
		USBERROR( "RTL8188 : Error allocating memory" );

		return ( NULL );
	}

	InitSemaphore( & unit->unit_CommandLock );

	NewList( & unit->unit_Pending );

	NewList( & unit->unit_MulticastRanges );

	NewList( & unit->unit_TrackedTypes );

	NewList( & unit->unit_Multicast );

	return ( unit );
}

// --
