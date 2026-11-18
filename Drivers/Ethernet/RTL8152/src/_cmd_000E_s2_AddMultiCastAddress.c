
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
	struct MulticastAddress *multicast;

	static const U8 broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	ioreq->ios2_WireError = 0;

	if ( ( ! IS_MULTICAST( ioreq->ios2_SrcAddr ) ) ||
		( memcmp( ioreq->ios2_SrcAddr, broadcast, sizeof( broadcast ) ) == 0 ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_BAD_MULTICAST;

		goto reply;
	}

	multicast = FindMulticastAddress( unit, ioreq->ios2_SrcAddr );

	if ( multicast )
	{
		if ( multicast->ma_UseCount == (U32) -1 )
		{
			ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
			ioreq->ios2_WireError = S2WERR_GENERIC_ERROR;
		}
		else
		{
			multicast->ma_UseCount++;
		}

		goto reply;
	}

	multicast = IExec->AllocVecTags( sizeof( *multicast ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! multicast )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		ioreq->ios2_WireError = S2WERR_GENERIC_ERROR;

		goto reply;
	}

	multicast->ma_UseCount = 1;

	memcpy( multicast->ma_Address, ioreq->ios2_SrcAddr, sizeof( multicast->ma_Address ) );

	IExec->AddTail( & unit->unit_MultiCastList, & multicast->ma_Node );

reply:

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
