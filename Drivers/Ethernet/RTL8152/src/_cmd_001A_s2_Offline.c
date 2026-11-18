
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
s2_Offline( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	if ( unit->unit_Online == FALSE )
	{
		ioreq->ios2_Req.io_Error= S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError	= S2WERR_UNIT_OFFLINE;
		goto bailout;
	}

	unit->unit_Online = FALSE;
	ioreq->ios2_WireError = 0;

	AbortPacketIO( unit );

	AbortAll( unit, NULL, S2WERR_UNIT_OFFLINE );

	CheckEvent( unit, S2EVENT_OFFLINE );

bailout:

	IExec->ReplyMsg( (APTR) ioreq );

}

// --
