
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
s2_GetStationAddress( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	memcpy( & ioreq->ios2_SrcAddr[0], & unit->unit_CurrentAddress[0], 6 );
	memcpy( & ioreq->ios2_DstAddr[0], & unit->unit_DefaultAddress[0], 6 );

	ReplySanaRequest( unit, (PTR) ioreq );
	
}

// --
