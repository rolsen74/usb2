
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
s2_DelMultiCastAddress( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	struct MulticastAddress *multicast;

	multicast = FindMulticastAddress( unit, ioreq->ios2_SrcAddr );

	ioreq->ios2_WireError = 0;

	if ( ! multicast )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_BAD_MULTICAST;
	}
	else if ( --multicast->ma_UseCount == 0 )
	{
		IExec->Remove( & multicast->ma_Node );

		IExec->FreeVec( multicast );
	}

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
