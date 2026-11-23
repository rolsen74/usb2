/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S8
Audio_Read_Reply( struct AudioUnit *unit, S8 error )
{
	struct IOStdReq *ioreq;

	ioreq = unit->ReadActive;

	if ( ! ioreq )
	{
		return ( error );
	}

	unit->ReadActive = NULL;

	ObtainSemaphore( & unit->Base->Semaphore );

	if ( ioreq->io_Flags & AUDIO_IO_ABORT )
	{
		error = IOERR_ABORTED;
	}

	ioreq->io_Error = ( ! error && unit->ReadDiscontinuity ) ? AUDIO_ERR_SHORT_READ : error;
	ioreq->io_Actual = unit->DataUsed;
	unit->DataUsed = 0;
	unit->ReadDiscontinuity = FALSE;

	ReplyMsg( (PTR) ioreq );

	ReleaseSemaphore( & unit->Base->Semaphore );

	return ( error );
}

// --
