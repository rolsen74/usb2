
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
SignalSana2Event( struct EthernetUnit *unit, U32 events )
{
	struct IOSana2Req *ioreq;
	struct IOSana2Req *next;

	ioreq = (PTR) GetHead( & unit->unit_Pending );

	while( ioreq )
	{
		next = (PTR) GetSucc( (PTR) ioreq );

		if ( ( ioreq->ios2_Req.io_Command == S2_ONEVENT ) && ( ioreq->ios2_WireError & events ) )
		{
			Remove( (PTR) ioreq );

			ioreq->ios2_WireError &= events;

			ReplyMsg( (PTR) ioreq );
		}

		ioreq = next;
	}
}

// --
