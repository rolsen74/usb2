
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
s2_Online( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	if ( unit->unit_Running == FALSE )
	{
		ioreq->ios2_Req.io_Error= S2ERR_BAD_STATE;
		ioreq->ios2_WireError	= S2WERR_UNIT_DISCONNECTED;
		goto bailout;
	}

	if ( unit->unit_Configured == FALSE )
	{

		ioreq->ios2_Req.io_Error= IOERR_SELFTEST;
		ioreq->ios2_WireError	= 0;
		goto bailout;
	}

	if ( unit->unit_ControlNeedsSetup )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_GENERIC_ERROR;

		goto bailout;
	}

	if ( unit->unit_Online )
	{
		ioreq->ios2_Req.io_Error= S2ERR_BAD_STATE;
		ioreq->ios2_WireError	= S2WERR_UNIT_ONLINE;
		goto bailout;
	}

	unit->unit_Online = TRUE;
	unit->unit_PacketsReceived = 0;
	unit->unit_PacketsSent = 0;
	unit->unit_BadData = 0;
	unit->unit_Overruns = 0;
	unit->unit_UnknownTypesReceived = 0;
	unit->unit_Reconfigurations = 0;
	unit->unit_MultiCastRejects = 0;

	ioreq->ios2_WireError = 0;

	ITimer->GetSysTime( &unit->unit_LastStart );

	unit->unit_RxRecovery = 0;
	unit->unit_TxRecovery = 0;

	RTL8152_UpdateLink( unit );

	Schedule_RxIOReq( unit );

	SignalSanaEvent( unit, S2EVENT_ONLINE | S2EVENT_CONFIGCHANGED );

bailout:

	ReplySanaRequest( unit, (PTR) ioreq );

}

// --
