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
	if ( ( unit->ReadActive ) && ( unit->ReadActive->io_Flags & VIDEO_IO_ABORT ) )
	{
		if ( unit->ReadPending )
		{
			Video_IO_Stop( unit->Stream->IORequests[0] );
			unit->ReadPending = FALSE;
		}

		Video_Read_Reply( unit, IOERR_ABORTED );
	}
}

// --
