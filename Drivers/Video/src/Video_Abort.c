/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Video_Abort( struct VideoUnit *unit )
{
	U32 reset;
	S8 error;

	ObtainSemaphore( & unit->Base->Semaphore );

	reset = unit->ReadReset;
	unit->ReadReset = FALSE;

	ReleaseSemaphore( & unit->Base->Semaphore );

	if ( reset )
	{
		error = Video_Stream_Close( unit );

		Video_Read_Reply( unit, error == USB2Err_Device_Detached ? error : IOERR_ABORTED );
	}

	if ( ( unit->ReadActive ) && ( unit->ReadActive->io_Flags & VIDEO_IO_ABORT ) )
	{
		Video_Read_Stop( unit );

		Video_Read_Reply( unit, IOERR_ABORTED );
	}
}

// --
