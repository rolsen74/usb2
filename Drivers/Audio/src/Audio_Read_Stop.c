
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
Audio_Read_Stop( struct AudioUnit *unit )
{
	struct USB2_IORequest *ioreq;

	U32 slot;
	U32 index;

	for( index = 0; index < unit->ReadCount; index++ )
	{
		slot = ( unit->ReadHead + index ) % unit->ReadSlots;

		if ( unit->Reads[slot].Pending )
		{
			ioreq = unit->Stream->IORequests[slot];

			if ( ! CheckIO( (PTR) ioreq ) )
			{
				AbortIO( (PTR) ioreq );
			}
		}
	}

	for( index = 0; index < unit->ReadCount; index++ )
	{
		slot = ( unit->ReadHead + index ) % unit->ReadSlots;

		if ( unit->Reads[slot].Pending )
		{
			WaitIO( (PTR) unit->Stream->IORequests[slot] );

			unit->Reads[slot].Pending = FALSE;
		}
	}

	unit->ReadHead = 0;
	unit->ReadCount = 0;
}

// --
