
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
Video_Read_Stop( struct VideoUnit *unit )
{
	struct USB2_IORequest *ioreq;

	S32 index;

	if ( unit->ReadPending )
	{
		for ( index = unit->ReadCount - 1; index >= 0; index-- )
		{
			if ( unit->ReadPending & ( 1U << index ) )
			{
				ioreq = unit->Stream->IORequests[index];

				if ( ! CheckIO( (PTR) ioreq ) )
				{
					AbortIO( (PTR) ioreq );
				}
			}
		}

		for ( index = 0; index < unit->ReadCount; index++ )
		{
			if ( unit->ReadPending & ( 1U << index ) )
			{
				WaitIO( (PTR) unit->Stream->IORequests[index] );
			}
		}
	}

	unit->ReadPending = 0;
	unit->ReadHead = 0;
	unit->ReadTail = 0;
	unit->ReadGaps = 0;
	unit->ReadReady = NULL;
	unit->ReadPacket = 0;
	unit->FrameUsed = 0;
	unit->FrameStarted = FALSE;
	unit->FrameDiscard = TRUE;
}

// --
