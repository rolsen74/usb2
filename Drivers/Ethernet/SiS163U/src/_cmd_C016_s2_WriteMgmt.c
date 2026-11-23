
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

S32
Cmd_WriteMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	const U8 *frame;

	frame = ioreq->ios2_Data;

	if ( ( ! unit->unit_ManualScan ) || ( ! unit->unit_RadioInitialized ) || ( ! unit->unit_RxReady ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;
	}
	else if ( ( ! frame ) || ( ! ioreq->ios2_BufferManagement ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else if ( ( ioreq->ios2_DataLength < SIS163U_WLAN_HEADER ) ||
		( ioreq->ios2_DataLength > SIS163U_TX_BUFFER_SIZE - SIS163U_TX_DESC_SIZE ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
	}
	else if ( ( frame[0] & 0x0f ) || ( frame[1] & 0x47 ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
	}
	else
	{
		AddTail( & unit->unit_Pending, (PTR) ioreq );

		Signal( unit->unit_TaskAdr, SIGBREAKF_CTRL_E );

		return ( TRUE );
	}

	return ( FALSE );
}

// --
