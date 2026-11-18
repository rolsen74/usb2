
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
Device_BeginIO( struct DeviceManagerInterface *Self UNUSED, struct IOSana2Req *ioreq )
{
struct devUnit *unit;

	unit = (APTR) ioreq->ios2_Req.io_Unit;

	// Disable QUICKIO - we cannot support that
	ioreq->ios2_Req.io_Flags &= ~IOF_QUICK;

	IExec->Forbid();

	if ( ( unit ) && ( unit->unit_Running ) )
	{
		IExec->PutMsg( unit->unit_BeginIOMsgPort, (PTR) ioreq );

		IExec->Permit();

		return;
	}

	IExec->Permit();

	ioreq->ios2_Req.io_Error = IOERR_UNITBUSY;

	IExec->ReplyMsg( (PTR) ioreq );

}

// --
