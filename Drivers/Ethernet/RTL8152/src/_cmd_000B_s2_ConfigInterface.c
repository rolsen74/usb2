
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
s2_ConfigInterface( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	static const U8 zero[6] = { 0 };

	if ( ( IS_MULTICAST( ioreq->ios2_SrcAddr ) ) ||
		( memcmp( ioreq->ios2_SrcAddr, zero, sizeof( zero ) ) == 0 ) )
    {
	    ioreq->ios2_Req.io_Error= S2ERR_BAD_ADDRESS;
	    ioreq->ios2_WireError   = 0;
	    goto bailout;
    }

	if ( ( unit->unit_Configured ) && ( unit->unit_Online ) )
    {
	    ioreq->ios2_Req.io_Error= IOERR_SELFTEST;
	    ioreq->ios2_WireError   = 0;
	    goto bailout;
    }

	if ( ! RTL8152_SetMacAddress( unit, ioreq->ios2_SrcAddr ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
		ioreq->ios2_WireError = S2WERR_GENERIC_ERROR;

		goto bailout;
	}

	memcpy( unit->unit_CurrentAddress, ioreq->ios2_SrcAddr, 6 );

	ITimer->GetSysTime( & unit->unit_LastStart );

	ioreq->ios2_WireError = 0;
	unit->unit_Configured = TRUE;
	unit->unit_Online = TRUE;

	Schedule_RxIOReq( unit );

	SignalSanaEvent( unit, S2EVENT_ONLINE | S2EVENT_CONFIGCHANGED, TRUE );

bailout:

	ReplySanaRequest( unit, (PTR) ioreq );

}

// --
