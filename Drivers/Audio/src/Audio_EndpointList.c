
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
Audio_EndpointList( struct AudioBase *base, struct IOStdReq *ioreq )
{
	struct AudioEndpoint entry;
	struct AudioUnit *unit;
	struct AudioUnit *match;

	U32 needed;
	U32 index;
	U8 *output;

	ioreq->io_Actual = 0;

	if ( ioreq->io_Offset != AUDIO_ENDPOINT_VERSION )
	{
		ioreq->io_Error = IOERR_NOCMD;

		return;
	}

	ObtainSemaphore( & base->Semaphore );

	needed = 0;

	for( index = 0; index < AUDIO_MAX_UNITS; index++ )
	{
		unit = base->Units[index];

		if ( ( unit ) && ( ! unit->Detached ) )
		{
			needed += sizeof( entry );
		}
	}

	ioreq->io_Actual = needed;

	if ( ( needed > ioreq->io_Length ) || ( ( needed ) && ( ! ioreq->io_Data ) ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;
	}
	else
	{
		ioreq->io_Error = 0;
		output = ioreq->io_Data;

		for( index = 0; index < AUDIO_MAX_UNITS; index++ )
		{
			unit = base->Units[index];

			if ( ( ! unit ) || ( unit->Detached ) )
			{
				continue;
			}

			CopyMem( & unit->Endpoint, & entry, sizeof( entry ) );

			entry.Flags = unit->OpenCount ? AUDIO_ENDPOINT_BUSY : 0;

			if ( ! Audio_EndpointFind( base, entry.ID, & match ) )
			{
				CopyMem( unit->PortID, entry.ID, sizeof( entry.ID ) );
			}

			CopyMem( & entry, output, sizeof( entry ) );

			output += sizeof( entry );
		}
	}

	ReleaseSemaphore( & base->Semaphore );
}

// --
