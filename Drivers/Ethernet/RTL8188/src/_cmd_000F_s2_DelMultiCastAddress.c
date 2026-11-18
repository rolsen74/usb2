
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
Cmd_DelMultiCastAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct MulticastAddress *entry;

	entry = FindMulticastAddress( unit, ioreq->ios2_SrcAddr );

	if ( ! entry )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_BAD_MULTICAST;
	}
	else if ( --entry->ma_UseCount == 0 )
	{
		Remove( & entry->ma_Node );

		FreeVec( entry );
	}
}

// --
