
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
Audio_Write_Stop( struct AudioUnit *unit, S8 error )
{
	struct USB2_IORequest *ioreq;

	U32 slot;

	for( slot = 0; slot < AUDIO_STREAM_REQUESTS; slot++ )
	{
		if ( unit->WriteOwners[slot] )
		{
			ioreq = unit->Stream->IORequests[slot];

			if ( ! CheckIO( (PTR) ioreq ) )
			{
				AbortIO( (PTR) ioreq );
			}
		}
	}

	for( slot = 0; slot < AUDIO_STREAM_REQUESTS; slot++ )
	{
		if ( unit->WriteOwners[slot] )
		{
			WaitIO( (PTR) unit->Stream->IORequests[slot] );

			Audio_Write_Account( unit, slot );
		}
	}

	unit->WritePhase = 0;

	while( unit->WriteCount )
	{
		Audio_Write_Reply( unit, error );
	}
}

// --
