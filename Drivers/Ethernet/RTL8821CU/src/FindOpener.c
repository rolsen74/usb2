
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct BufferManagement *
FindOpener( struct EthernetUnit *unit, PTR cookie )
{
	struct Node *node;

	for( node = GetHead( & unit->unit_Openers ); node; node = GetSucc( node ) )
	{
		if ( node == cookie )
		{
			return ( (PTR) node );
		}
	}

	return ( NULL );
}

// --
