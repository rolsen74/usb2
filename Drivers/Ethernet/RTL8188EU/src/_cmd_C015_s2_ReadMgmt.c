
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

S32
Cmd_ReadMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	if ( ! unit->unit_RadioReady )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;
	}
	else if ( ! ioreq->ios2_Data )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else if ( ioreq->ios2_DataLength < 24 )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
	}
	else
	{
		AddTail( & unit->unit_Pending, (PTR) ioreq );

		RTL8188EU_SubmitRX( unit );

		return ( TRUE );
	}

	return ( FALSE );
}

// --
