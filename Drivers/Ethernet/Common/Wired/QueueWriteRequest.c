
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
QueueWriteRequest( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	if ( ( ! unit->unit_LinkUp ) || ( unit->unit_TxHalted ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = unit->unit_LinkUp ? S2WERR_GENERIC_ERROR : S2WERR_UNIT_DISCONNECTED;

		ReplySanaRequest( unit, ioreq );

		return;
	}

	// Add to unit Tx list
	IExec->AddTail( & unit->unit_TxList, (APTR) ioreq );

	Schedule_TxIOReq( unit );

}

// --
