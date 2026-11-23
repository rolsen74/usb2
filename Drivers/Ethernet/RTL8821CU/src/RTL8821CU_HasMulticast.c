
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
RTL8821CU_HasMulticast( struct EthernetUnit *unit, const U8 *address )
{
	struct BufferManagement *opener;

	for( opener = (PTR) GetHead( & unit->unit_Openers ); opener; opener = (PTR) GetSucc( & opener->bm_Node ) )
	{
		if ( Sana2_MulticastMatch( & opener->bm_Multicast, address ) )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --
