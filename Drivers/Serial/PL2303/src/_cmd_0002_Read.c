/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Cmd_Read( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	if ( ( unit->Stopped ) || ( ! ioreq->IOSer.io_Data ) ||
		( ! ioreq->IOSer.io_Length ) || ( ioreq->IOSer.io_Length == (U32) -1 ) )
	{
		ioreq->IOSer.io_Error = SerErr_InvParam;
		Serial_Reply( ctx, ioreq );

		return;
	}

	ioreq->IOSer.io_Flags |= IOSERF_QUEUED;
	AddTail( & unit->ReadList, (PTR) ioreq );

	Serial_Read_Drain( ctx, unit );
}

// --
