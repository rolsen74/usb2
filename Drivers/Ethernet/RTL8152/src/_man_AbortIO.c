
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
Device_AbortIO( struct DeviceManagerInterface *Self UNUSED, struct IOSana2Req *ioreq )
{
struct AbortIOMessage *msg;
struct devUnit *unit;

	unit = (APTR) ioreq->ios2_Req.io_Unit;

	msg = IExec->AllocVecTags( sizeof( struct AbortIOMessage ),
		AVT_ClearWithValue, 0,
		TAG_END
	);

	if ( ! msg )
	{
		return;
	}

	IExec->Forbid();

	if ( ( unit ) && ( unit->unit_Running ) && ( ! IExec->CheckIO( (PTR) ioreq ) ) )
	{
		msg->ioreq = ioreq;

		IExec->PutMsg( unit->unit_AbortIOMsgPort, (PTR) msg );

		IExec->Permit();

		return;
	}

	IExec->Permit();

	IExec->FreeVec( msg );

}

// --
