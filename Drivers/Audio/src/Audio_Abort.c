/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Audio_Abort( struct AudioUnit *unit )
{
	if ( ( unit->ReadActive ) && ( unit->ReadActive->io_Flags & AUDIO_IO_ABORT ) )
	{
		if ( unit->ReadPending )
		{
			Audio_IO_Stop( unit->Stream->IORequests[0] );
			unit->ReadPending = FALSE;
		}

		Audio_Write_Stop( unit );

		Audio_Read_Reply( unit, IOERR_ABORTED );
		unit->BufferedBytes = 0;
		unit->BufferOffset = 0;
		unit->WritePhase = 0;
	}
}

// --
