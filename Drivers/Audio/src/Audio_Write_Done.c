
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

	U32 frameBytes;
	U32 actual;
	U32 slot;
	U32 bytes;

	while(( ioreq = (PTR) GetMsg( unit->Stream->MsgPort ) ))
	{
		slot = ioreq == unit->Stream->IORequests[0] ? 0 : 1;
		bytes = unit->WriteBytes[slot];
		unit->WriteBytes[slot] = 0;
		write = unit->ReadActive;

		if ( ! write )
		{
			continue;
		}

		if ( write->io_Flags & AUDIO_IO_ABORT )
		{
			Audio_Write_Stop( unit );

			unit->WritePhase = 0;

			Audio_Read_Reply( unit, IOERR_ABORTED );

			continue;
		}

		if ( ioreq->io_Error != USB2Err_NoError )
		{
			AUDIO_TRACE( "unit %lu endpoint $%02lx write failed, USB error %ld, bytes %lu\n",
				unit->UnitNr, (U32) unit->EndPoint, (S32) ioreq->io_Error, ioreq->io_Actual );

			Audio_Write_Stop( unit );

			unit->WritePhase = 0;

			Audio_Read_Reply( unit, ioreq->io_Error );

			continue;
		}

		if ( ioreq->io_Actual != bytes )
		{
			AUDIO_TRACE( "unit %lu endpoint $%02lx wrote %lu of %lu USB bytes\n",
				unit->UnitNr, (U32) unit->EndPoint, ioreq->io_Actual, bytes );

			frameBytes = unit->Format.Channels * 2;
			actual = MIN( ioreq->io_Actual, bytes );
			unit->WriteLostBytes += bytes - ( actual - actual % frameBytes );
		}

		unit->DataUsed += bytes;

		if ( unit->DataUsed == write->io_Length )
		{
			Audio_Read_Reply( unit, unit->WriteLostBytes ? AUDIO_ERR_SHORT_WRITE : 0 );
		}
	}

	Audio_Write_Start( unit );
}

// --
