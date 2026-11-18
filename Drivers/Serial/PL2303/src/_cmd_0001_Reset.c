/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Cmd_Reset( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	Serial_Flush( ctx, unit );

	unit->RxHead = 0;
	unit->RxTail = 0;
	unit->Stopped = FALSE;

	if ( unit->WriteActive )
	{
		unit->WriteActive->IOSer.io_Flags |= IOSERF_ABORT;
		AbortIO( (PTR) unit->BulkOut->IORequests[0] );
	}

	Serial_Reply( ctx, ioreq );
}

// --
