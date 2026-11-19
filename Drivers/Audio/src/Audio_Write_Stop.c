
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
Audio_Write_Stop( struct AudioUnit *unit )
{
	struct USB2_IORequest *ioreq;

	U32 slot;

	for ( slot = 0; slot < AUDIO_WRITE_REQUESTS; slot++ )
	{
		if ( unit->WriteBytes[slot] )
		{
			ioreq = unit->Stream->IORequests[slot];

			if ( ! CheckIO( (PTR) ioreq ) )
			{
				AbortIO( (PTR) ioreq );
			}
		}
	}

	for ( slot = 0; slot < AUDIO_WRITE_REQUESTS; slot++ )
	{
		if ( unit->WriteBytes[slot] )
		{
			WaitIO( (PTR) unit->Stream->IORequests[slot] );

			unit->WriteBytes[slot] = 0;
		}
	}
}

// --
