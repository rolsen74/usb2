
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
	// Add to unit Tx list
	IExec->AddTail( & unit->unit_TxList, (APTR) ioreq );

	Schedule_TxIOReq( unit );

}

// --
