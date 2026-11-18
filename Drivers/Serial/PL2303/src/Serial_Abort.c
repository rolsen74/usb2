/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Abort( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct IOExtSer *ioreq;
struct IOExtSer *next;

	if ( ( unit->WriteActive ) && ( unit->WriteActive->IOSer.io_Flags & IOSERF_ABORT ) )
	{
		AbortIO( (PTR) unit->BulkOut->IORequests[0] );
	}

	ioreq = (PTR) GetHead( & unit->ReadList );

	while( ioreq )
	{
		next = (PTR) GetSucc( (PTR) ioreq );

		if ( ioreq->IOSer.io_Flags & IOSERF_ABORT )
		{
			Remove( (PTR) ioreq );
			ioreq->IOSer.io_Error = IOERR_ABORTED;
			Serial_Reply( ctx, ioreq );
		}

		ioreq = next;
	}

	ioreq = (PTR) GetHead( & unit->WriteList );

	while( ioreq )
	{
		next = (PTR) GetSucc( (PTR) ioreq );

		if ( ioreq->IOSer.io_Flags & IOSERF_ABORT )
		{
			Remove( (PTR) ioreq );
			ioreq->IOSer.io_Error = IOERR_ABORTED;
			Serial_Reply( ctx, ioreq );
		}

		ioreq = next;
	}
}

// --
