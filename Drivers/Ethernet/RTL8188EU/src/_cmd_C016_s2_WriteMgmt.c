
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Commands.h"
#include "Radio.h"

// --

S32
Cmd_WriteMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	U8 *frame;

	frame = ioreq->ios2_Data;

	if ( ! unit->unit_TxReady )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;
	}
	else if ( ( ioreq->ios2_DataLength < 24 ) || ( ioreq->ios2_DataLength > RTL8188EU_FRAME_MAX ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
	}
	else if ( ( ! frame ) || ( frame[0] & 0x0f ) || ( frame[1] & 0x43 ) ||
		( memcmp( frame + 10, unit->unit_StationAddress, 6 ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_INVALID_OPTION;
	}
	else
	{
		RTL8188EU_QueueControl( unit, ioreq );

		return ( TRUE );
	}

	return ( FALSE );
}

// --
