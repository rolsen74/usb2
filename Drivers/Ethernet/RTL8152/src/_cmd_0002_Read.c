
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

void
Cmd_Read( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	if ( unit->unit_Running == FALSE )
	{
		SHOWMSG( "Not Running" );

		ioreq->ios2_Req.io_Error= S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError	= S2WERR_UNIT_OFFLINE;
		goto bailout;
	}

	if ( unit->unit_Online == FALSE )
	{
		SHOWMSG( "Offline" );

		ioreq->ios2_Req.io_Error= S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError	= S2WERR_UNIT_OFFLINE;
		goto bailout;
	}

	// Reads always go on a list and are never IOF_QUICK as they get replied upon reception.
	// Broadcast or multicast is also unknown at this point.
	ioreq->ios2_Req.io_Flags &= ~( SANA2IOF_BCAST | SANA2IOF_MCAST );

	// Add I/O request to the pending reads list of the stack queue.
	ioreq->ios2_Req.io_Message.mn_Node.ln_Type	= 0;
	ioreq->ios2_Req.io_Error					= 0;

	QueueReadRequest( unit, ioreq );

	// Don't try to reply this!
	ioreq = NULL;

bailout:

	if ( ioreq )
	{
		IExec->ReplyMsg( (APTR) ioreq );
	}

}

// --
