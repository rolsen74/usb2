
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
AbortPendingCommands( struct EthernetUnit *unit, struct IORequest *request, struct BufferManagement *opener )
{
	struct IOSana2Req *ioreq;
	struct IOSana2Req *next;

	ioreq = (PTR) GetHead( & unit->unit_Pending );

	while( ioreq )
	{
		next = (PTR) GetSucc( (PTR) ioreq );

		if ( ( ( ! request ) || ( & ioreq->ios2_Req == request ) ) &&
			( ( ! opener ) || ( ioreq->ios2_BufferManagement == opener ) ) )
		{
			Remove( (PTR) ioreq );

			ioreq->ios2_Req.io_Error = IOERR_ABORTED;
			ioreq->ios2_WireError = 0;

			ReplyMsg( (PTR) ioreq );

			if ( request )
			{
				break;
			}
		}

		ioreq = next;
	}
}

// --
