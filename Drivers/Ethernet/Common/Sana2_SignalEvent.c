
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/exec.h>
#include "Sana2Commands.h"

// --

extern struct ExecIFace *IExec;

// --

void
Sana2_SignalEventWithReply( struct List *list, uint32 events, int return_all, Sana2Reply reply, APTR context )
{
	struct IOSana2Req *ioreq;
	struct IOSana2Req *next;

	ioreq = (APTR) IExec->GetHead( list );

	while( ioreq )
	{
		next = (APTR) IExec->GetSucc( (APTR) ioreq );

		if ( ( ioreq->ios2_Req.io_Command == S2_ONEVENT ) && ( ioreq->ios2_WireError & events ) )
		{
			IExec->Remove( (APTR) ioreq );

			ioreq->ios2_WireError = return_all ? events : ioreq->ios2_WireError & events;

			if ( reply )
			{
				reply( context, ioreq );
			}
			else
			{
				IExec->ReplyMsg( (APTR) ioreq );
			}
		}

		ioreq = next;
	}
}

// --

void
Sana2_SignalEvent( struct List *list, uint32 events, int return_all )
{
	Sana2_SignalEventWithReply( list, events, return_all, NULL, NULL );
}

// --
