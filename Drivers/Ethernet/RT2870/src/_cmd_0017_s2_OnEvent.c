
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

S32
Cmd_OnEvent( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	U32 events;
	U32 supported;

	events = ioreq->ios2_WireError;
	supported = S2EVENT_ONLINE | S2EVENT_OFFLINE | S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX | S2EVENT_TX | S2EVENT_BUFF;

	if ( ( ! events ) || ( events & ~supported ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
		ioreq->ios2_WireError = S2WERR_BAD_EVENT;

		return ( FALSE );
	}

	if ( events & ( unit->unit_Online ? S2EVENT_ONLINE : S2EVENT_OFFLINE ) )
	{
		ioreq->ios2_WireError = unit->unit_Online ? S2EVENT_ONLINE : S2EVENT_OFFLINE;

		return ( FALSE );
	}

	AddTail( & unit->unit_Pending, (PTR) ioreq );

	return ( TRUE );
}

// --
