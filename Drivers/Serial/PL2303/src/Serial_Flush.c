/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Flush( struct SerialContext *ctx, struct SerialUnit *unit, BOOL active )
{
struct IOExtSer *ioreq;

	if ( ( active ) && ( unit->ReadActive ) )
	{
		ioreq = unit->ReadActive;
		unit->ReadActive = NULL;
		ioreq->IOSer.io_Error = IOERR_ABORTED;

		Serial_Reply( ctx, ioreq );
	}

	while(( ioreq = (PTR) RemHead( & unit->ReadList ) ))
	{
		ioreq->IOSer.io_Error = IOERR_ABORTED;
		ioreq->IOSer.io_Flags |= IOSERF_ABORT;
		Serial_Reply( ctx, ioreq );
	}

	while(( ioreq = (PTR) RemHead( & unit->WriteList ) ))
	{
		ioreq->IOSer.io_Error = IOERR_ABORTED;
		ioreq->IOSer.io_Flags |= IOSERF_ABORT;
		Serial_Reply( ctx, ioreq );
	}
}

// --
