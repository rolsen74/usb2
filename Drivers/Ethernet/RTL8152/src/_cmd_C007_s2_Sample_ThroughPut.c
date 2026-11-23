
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
s2_Sample_ThroughPut( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	ioreq->ios2_WireError = 0;

	if ( ! Sana2_QueueThroughput( & unit->unit_SampleList, ioreq, ITimer, TRUE ) )
	{
		IExec->ReplyMsg( (PTR) ioreq );
	}
}

// --
