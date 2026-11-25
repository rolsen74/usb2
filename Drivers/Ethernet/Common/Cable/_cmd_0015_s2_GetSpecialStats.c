
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
s2_GetSpecialStats( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	struct Sana2SpecialStatRecord record;

	ioreq->ios2_WireError = 0;
	record.Type = S2SS_ETHERNET_BADMULTICAST;
	record.Count = unit->unit_MultiCastRejects;
	record.String = "Rejected multicast packets";

	Sana2_GetSpecialStats( ioreq, & record );

	ReplySanaRequest( unit, (PTR) ioreq );
}

// --
