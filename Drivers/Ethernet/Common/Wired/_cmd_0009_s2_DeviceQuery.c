
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
s2_DeviceQuery( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	struct Sana2DeviceQuery *s2_query;
	struct Sana2DeviceQuery info;

	info = (struct Sana2DeviceQuery)
	{
		.AddrFieldSize = 48,
		.MTU = ETHERNET_PAYLOAD_MTU,
		.BPS = unit->unit_Support_GMII ? 1000000000UL : 100000000UL,
		.HardwareType = S2WireType_Ethernet,
		.RawMTU = ETHERNET_MTU
	};

	s2_query = ioreq->ios2_StatData;
	ioreq->ios2_WireError = 0;

	if ( s2_query == NULL )
	{
		SHOWMSG( "no stat data pointer provided" );

		ioreq->ios2_Req.io_Error= S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError	= S2WERR_NULL_POINTER;

		goto bailout;
	}

	if ( s2_query->SizeAvailable < offsetof( struct Sana2DeviceQuery, AddrFieldSize ))
	{
		SHOWMSG( "query buffer is too short" );

		ioreq->ios2_Req.io_Error= S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError	= S2WERR_BAD_STATDATA;

		goto bailout;
	}

	s2_query->SizeSupplied = s2_query->SizeAvailable;

	if ( s2_query->SizeSupplied > sizeof( info ))
    {
		s2_query->SizeSupplied = sizeof( info );
	}

	memcpy( &s2_query->DevQueryFormat, &info.DevQueryFormat,
		s2_query->SizeSupplied - offsetof( struct Sana2DeviceQuery, DevQueryFormat ));

bailout:

	ReplySanaRequest( unit, (PTR) ioreq );

}

// --
