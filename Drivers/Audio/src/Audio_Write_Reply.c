
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
Audio_Write_Reply( struct AudioUnit *unit, S8 error )
{
	struct IOStdReq *request;
	struct AudioWrite *write;

	write = & unit->Writes[unit->WriteHead];
	request = write->Request;
	request->io_Actual = write->Actual;
	unit->WriteHead = ( unit->WriteHead + 1 ) % AUDIO_STREAM_REQUESTS;
	unit->WriteCount--;

	ObtainSemaphore( & unit->Base->Semaphore );

	if ( request->io_Flags & AUDIO_IO_ABORT )
	{
		error = IOERR_ABORTED;
	}
	else if ( ( ! error ) && ( request->io_Actual != request->io_Length ) )
	{
		error = AUDIO_ERR_SHORT_WRITE;
	}

	request->io_Error = error;

	ReplyMsg( (PTR) request );

	ReleaseSemaphore( & unit->Base->Semaphore );

	return ( error );
}

// --
