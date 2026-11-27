/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void
Audio_Abort( struct AudioUnit *unit )
{
	struct AudioWrite *write;

	U32 index;

	if ( ( unit->ReadActive ) && ( unit->ReadActive->io_Flags & AUDIO_IO_ABORT ) )
	{
		Audio_Read_Stop( unit );

		Audio_Read_Reply( unit, IOERR_ABORTED );
	}

	for( index = 0; index < unit->WriteCount; index++ )
	{
		write = & unit->Writes[( unit->WriteHead + index ) % AUDIO_WRITE_REQUESTS];

		if ( write->Request->io_Flags & AUDIO_IO_ABORT )
		{
			Audio_Write_Stop( unit, IOERR_ABORTED );

			break;
		}
	}
}

// --
