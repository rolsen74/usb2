
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "All.h"
#include "Commands.h"

// --

static const struct Sana2DeviceQuery Info =
{
	.AddrFieldSize = 48,
	.MTU = RTL8811AU_MTU,
	.RawMTU = RTL8811AU_MTU + RTL8811AU_ETH_HEADER,
	.HardwareType = S2WireType_Ethernet
};

// --

void
Cmd_DeviceQuery( struct IOSana2Req *ioreq )
{
	struct Sana2DeviceQuery *query;

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

	query->SizeSupplied = MIN( available, sizeof( Info ) );

	CopyMem( (PTR) & Info.DevQueryFormat, & query->DevQueryFormat,
		query->SizeSupplied - offsetof( struct Sana2DeviceQuery, DevQueryFormat ) );
}

// --
