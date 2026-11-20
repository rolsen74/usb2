
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
Cmd_MultiCast( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	if ( ! ValidMulticastAddress( ioreq->ios2_DstAddr ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_BAD_MULTICAST;

		return;
	}

	Cmd_Write( unit, ioreq );
}

// --
