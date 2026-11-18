
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

struct MulticastAddress *
FindMulticastAddress( struct EthernetUnit *unit, const U8 *address )
{
	struct MulticastAddress *entry;

	entry = (PTR) GetHead( & unit->unit_Multicast );

	while( entry )
	{
		if ( ! memcmp( entry->ma_Address, address, sizeof( entry->ma_Address ) ) )
		{
			break;
		}

		entry = (PTR) GetSucc( & entry->ma_Node );
	}

	return ( entry );
}

// --
