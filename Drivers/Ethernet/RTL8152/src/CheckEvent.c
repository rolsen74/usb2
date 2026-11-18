
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
CheckEvent( struct devUnit *unit, U32 events )
{
struct IOSana2Req *ioreq, *next;

	ioreq = (APTR) IExec->GetHead( & unit->unit_EventsList );

	while( ioreq )
	{
		next = (APTR) IExec->GetSucc( (APTR) ioreq );

		if ( ioreq->ios2_WireError & events )
		{
			ioreq->ios2_WireError = events;

			IExec->Remove( (APTR) ioreq );

			IExec->ReplyMsg( (APTR) ioreq );
		}

		ioreq = next;
	}

}

// --
