
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
	struct IOStdReq *write;

	while(( ioreq = (PTR) GetMsg( unit->Stream->MsgPort ) ))
	{
		unit->ReadPending = FALSE;
		write = unit->ReadActive;

		if ( ! write )
		{
			continue;
		}

		if ( write->io_Flags & AUDIO_IO_ABORT )
		{
			unit->WritePhase = 0;

			Audio_Read_Reply( unit, IOERR_ABORTED );

			continue;
		}

		if ( ioreq->io_Error != USB2Err_NoError )
		{
			AUDIO_TRACE( "unit %lu endpoint $%02lx write failed, USB error %ld, bytes %lu\n",
				unit->UnitNr, (U32) unit->EndPoint, (S32) ioreq->io_Error, ioreq->io_Actual );

			unit->WritePhase = 0;

			Audio_Read_Reply( unit, ioreq->io_Error );

			continue;
		}

		if ( ioreq->io_Actual != unit->WriteBytes )
		{
			AUDIO_TRACE( "unit %lu endpoint $%02lx wrote %lu of %lu USB bytes\n",
				unit->UnitNr, (U32) unit->EndPoint, ioreq->io_Actual, unit->WriteBytes );

			unit->WritePhase = 0;

			Audio_Read_Reply( unit, IOERR_BADLENGTH );

			continue;
		}

		unit->DataUsed += unit->WriteBytes;

		if ( unit->DataUsed == write->io_Length )
		{
			Audio_Read_Reply( unit, 0 );
		}
	}

	Audio_Write_Start( unit );
}

// --
