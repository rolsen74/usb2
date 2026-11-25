
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
s2_MultiCast( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	if (( !IS_MULTICAST( ioreq->ios2_DstAddr ))
	||	(  IS_BROADCAST( ioreq->ios2_DstAddr )))
	{
		// Multicast writes aren't accepted for non-multicast addresses
		ioreq->ios2_Req.io_Error= S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError	= S2WERR_BAD_MULTICAST;

		ReplySanaRequest( unit, (PTR) ioreq );
		goto bailout;
	}

	// Just like any other write.
	Cmd_Write( unit, ioreq );

bailout:

}

// --
