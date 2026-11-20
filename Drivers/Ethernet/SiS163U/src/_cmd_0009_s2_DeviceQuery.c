
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
Cmd_DeviceQuery( struct EthernetUnit *unit UNUSED, struct IOSana2Req *ioreq )
{
	struct Sana2DeviceQuery *query;
	static const struct Sana2DeviceQuery info =
	{
		.AddrFieldSize = 48,
		.MTU = SIS163U_MTU,
		.RawMTU = SIS163U_MTU + 14,
		.BPS = 54000000,
		.HardwareType = S2WireType_Ethernet,
	};

	U32 available;

	query = ioreq->ios2_StatData;

	if ( ! query )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	available = query->SizeAvailable;

	if ( available < offsetof( struct Sana2DeviceQuery, AddrFieldSize ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_BAD_STATDATA;

		return;
	}

	query->SizeSupplied = available < sizeof( info ) ? available : sizeof( info );

	memcpy( & query->DevQueryFormat, & info.DevQueryFormat,
		query->SizeSupplied - offsetof( struct Sana2DeviceQuery, DevQueryFormat ) );
}

// --
