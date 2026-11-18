
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "Radio.h"

// --

void
Cmd_GetCryptTypes( struct EthernetUnit *unit UNUSED, struct IOSana2Req *ioreq )
{
	static const U8 types[] = { S2ENC_NONE, S2ENC_CCMP, S2ENC_TKIP };

	U8 *result;
	U32 count;

	ioreq->ios2_DataLength = 0;
	ioreq->ios2_StatData = NULL;

	if ( ! ioreq->ios2_Data )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	count = IRTL8188Timer ? sizeof( types ) : sizeof( types ) - 1;
	result = AllocPooled( ioreq->ios2_Data, count );

	if ( ! result )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;

		return;
	}

	memcpy( result, types, count );

	ioreq->ios2_DataLength = count;
	ioreq->ios2_StatData = result;
}

// --
