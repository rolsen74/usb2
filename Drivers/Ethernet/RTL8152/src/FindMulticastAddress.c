
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct MulticastAddress *
FindMulticastAddress( struct devUnit *unit, const U8 *address )
{
struct MulticastAddress *multicast;

	multicast = (APTR) IExec->GetHead( & unit->unit_MultiCastList );

	while( multicast )
	{
		if ( memcmp( multicast->ma_Address, address, sizeof( multicast->ma_Address )) == 0 )
		{
			break;
		}
		else
		{
			multicast = (APTR) IExec->GetSucc( (APTR) multicast );
		}
	}

	return( multicast );
}

// --
