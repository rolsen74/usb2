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
Video_Read_Done( struct VideoUnit *unit )
{
	struct USB2_IORequest *ioreq;

	U32 length;
	U32 count;
	S32 consumed;

	while ( unit->ReadActive )
	{
		if ( unit->ReadActive->io_Flags & VIDEO_IO_ABORT )
		{
			Video_Read_Stop( unit );

			Video_Read_Reply( unit, IOERR_ABORTED );
			break;
		}

		ioreq = unit->ReadReady;

		if ( ! ioreq )
		{
			ioreq = (PTR) GetMsg( unit->StreamPort );

			if ( ! ioreq )
			{
				break;
			}

			unit->ReadPending &= ~( 1U << ioreq->io_UserValue );

			if ( ioreq->io_Error )
			{
				Video_Read_Stop( unit );

				if ( ( unit->StreamAlt ) || ( ioreq->io_Error != USB2Err_Host_Stall ) ||
					( ! USB2_EPRes_Destall( unit->Stream ) ) )
				{
					Video_Read_Reply( unit, IOERR_UNITBUSY );
				}

				break;
			}

			if ( ( ! ioreq->io_Data ) || ( ioreq->io_Actual > ioreq->io_Length ) ||
				( ( unit->StreamAlt ) && ( ioreq->io_Length < unit->StreamPacketSize * unit->IsoPacketCount ) ) )
			{
				Video_Read_Stop( unit );

				Video_Read_Reply( unit, IOERR_BADLENGTH );
				break;
			}

			unit->ReadReady = ioreq;
			unit->ReadPacket = 0;
		}

		if ( unit->StreamAlt )
		{
			count = unit->IsoPacketCount;
			length = ioreq->io_IsoPacketLengths[unit->ReadPacket];

			if ( length == USB2_ISO_PACKET_ERROR )
			{
				consumed = Video_Read_Payload( unit, NULL, 1 );
			}
			else if ( length > unit->StreamPacketSize )
			{
				Video_Read_Stop( unit );

				Video_Read_Reply( unit, IOERR_BADLENGTH );
				break;
			}
			else
			{
				consumed = Video_Read_Payload( unit,
					(U8 *) ioreq->io_Data + unit->ReadPacket * unit->StreamPacketSize, length );
			}
		}
		else
		{
			count = 1;

			consumed = Video_Read_Payload( unit, ioreq->io_Data, ioreq->io_Actual );
		}

		if ( consumed )
		{
			unit->ReadPacket++;

			if ( unit->ReadPacket == count )
			{
				unit->ReadReady = NULL;

				Video_Read_Start( unit );
			}
		}
	}

	Video_Read_Start( unit );
}

// --
