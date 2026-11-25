
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
AbortPacketIO( struct devUnit *unit, U32 wire )
{
	struct List *lists[2];
	struct IONode *node;

	U32 index;

	lists[0] = & unit->unit_USBBulkInActiveList;
	lists[1] = & unit->unit_USBBulkOutActiveList;

	WIRED_RESET_RX( unit );

	for( index = 0; index < 2; index++ )
	{
		node = (PTR) IExec->GetHead( lists[index] );

		while( node )
		{
			if ( ! node->AbortError )
			{
				node->AbortError = S2ERR_OUTOFSERVICE;
				node->AbortWire = wire;

				IExec->AbortIO( (PTR) node->USBRequest );
			}

			node = (PTR) IExec->GetSucc( & node->Node );
		}
	}
}

// --
