
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
Cmd_Write( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	if ( unit->unit_Running == FALSE )
	{
		ioreq->ios2_Req.io_Error= S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError	= S2WERR_UNIT_OFFLINE;
		goto bailout;
	}

	if ( unit->unit_Online == FALSE )
	{
		ioreq->ios2_Req.io_Error= S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError	= S2WERR_UNIT_OFFLINE;
		goto bailout;
	}

	QueueWriteRequest( unit, ioreq );

	// Don't try to reply this!
	ioreq = NULL;

bailout:

	if ( ioreq )
	{
		IExec->ReplyMsg( (APTR) ioreq );
	}

}

// --
