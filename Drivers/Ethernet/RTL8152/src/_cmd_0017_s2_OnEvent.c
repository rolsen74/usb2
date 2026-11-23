
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
s2_OnEvent( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	U32 current;

	current = unit->unit_Online ? S2EVENT_ONLINE : S2EVENT_OFFLINE;

	if ( ! Sana2_QueueEvent( & unit->unit_EventsList, ioreq, current, SANA2_COMMON_EVENTS | S2EVENT_CONFIGCHANGED ) )
	{
		IExec->ReplyMsg( (PTR) ioreq );
	}
}

// --
