
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
s2_AddMultiCastAddress( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	Sana2_MulticastAddress( & unit->unit_MultiCastList, ioreq, S2WERR_GENERIC_ERROR );

	ReplySanaRequest( unit, (PTR) ioreq );
}

// --
