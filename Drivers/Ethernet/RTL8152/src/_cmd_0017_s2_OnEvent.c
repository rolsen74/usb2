
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
	U32 requested;
	U32 current;
	U32 supported;

	requested = ioreq->ios2_WireError;
	current = unit->unit_Online ? S2EVENT_ONLINE : S2EVENT_OFFLINE;
	supported = S2EVENT_ONLINE | S2EVENT_OFFLINE | S2EVENT_ERROR |
		S2EVENT_RX | S2EVENT_TX | S2EVENT_BUFF | S2EVENT_HARDWARE |
		S2EVENT_CONFIGCHANGED;

	if ( ( ! requested ) || ( requested & ~supported ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
		ioreq->ios2_WireError = S2WERR_BAD_EVENT;
	}
	else if ( requested & current )
	{
		ioreq->ios2_WireError = current;
	}
	else
	{
		IExec->AddTail( & unit->unit_EventsList, (PTR) ioreq );

		return;
	}

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
