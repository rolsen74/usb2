
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
Cmd_AddMultiCastAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct MulticastAddress *entry;

	if ( ! ValidMulticastAddress( ioreq->ios2_SrcAddr ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_BAD_MULTICAST;

		return;
	}

	entry = FindMulticastAddress( unit, ioreq->ios2_SrcAddr );

	if ( ! entry )
	{
		entry = AllocVecTags( sizeof( *entry ), AVT_ClearWithValue, 0, TAG_END );

		if ( ! entry )
		{
			ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
			ioreq->ios2_WireError = S2WERR_MULTICAST_FULL;

			return;
		}

		memcpy( entry->ma_Address, ioreq->ios2_SrcAddr, sizeof( entry->ma_Address ) );

		AddTail( & unit->unit_Multicast, & entry->ma_Node );
	}

	if ( entry->ma_UseCount == UINT32_MAX )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		ioreq->ios2_WireError = S2WERR_MULTICAST_FULL;

		return;
	}

	entry->ma_UseCount++;
}

// --
