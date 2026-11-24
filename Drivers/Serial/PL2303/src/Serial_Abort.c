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

	if ( ( unit->BreakRequest ) && ( unit->BreakRequest->IOSer.io_Flags & IOSERF_ABORT ) )
	{
		Serial_BreakStop( ctx, unit, IOERR_ABORTED );
	}

	if ( ( unit->WriteActive ) && ( unit->WriteActive->IOSer.io_Flags & IOSERF_ABORT ) )
	{
		if ( unit->WritePending )
		{
			AbortIO( (PTR)unit->BulkOut->IORequests[0] );
		}
		else
		{
			ioreq = unit->WriteActive;
			unit->WriteActive = NULL;

			Serial_Reply( ctx, ioreq );
		}
	}

	if ( ( unit->ReadActive ) && ( unit->ReadActive->IOSer.io_Flags & IOSERF_ABORT ) )
	{
		ioreq = unit->ReadActive;
		unit->ReadActive = NULL;

		Serial_Reply( ctx, ioreq );
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

	Serial_Read_Drain( ctx, unit );

	Serial_Write_Start( ctx, unit );
}

// --
