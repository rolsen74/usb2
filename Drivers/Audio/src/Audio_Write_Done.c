
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
Audio_Write_Done( struct AudioUnit *unit )
{
	struct USB2_IORequest *ioreq;
	struct AudioWrite *write;

	U32 slot;

	while(( ioreq = (PTR) GetMsg( unit->Stream->MsgPort ) ))
	{
		slot = ( ioreq == unit->Stream->IORequests[0] ) ? 0 : 1;

		Audio_Write_Account( unit, slot );

		if ( ioreq->io_Error != USB2Err_NoError )
		{
			AUDIO_TRACE( "unit %lu endpoint $%02lx write failed, USB error %ld, bytes %lu\n",
				unit->UnitNr, (U32) unit->EndPoint, (S32) ioreq->io_Error, ioreq->io_Actual );

			Audio_Write_Stop( unit, ioreq->io_Error );

			return;
		}

		while( unit->WriteCount )
		{
			write = & unit->Writes[unit->WriteHead];

			if ( write->Done != write->Request->io_Length )
			{
				break;
			}

			if ( Audio_Write_Reply( unit, 0 ) == IOERR_ABORTED )
			{
				Audio_Write_Stop( unit, IOERR_ABORTED );

				return;
			}
		}
	}

	Audio_Write_Start( unit );
}

// --
