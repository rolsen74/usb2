/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Video_Read_Done( struct VideoUnit *unit )
{
struct USB2_IORequest *ioreq;
struct IOStdReq *read;

U32 index;

	while(( ioreq = (PTR) GetMsg( unit->Stream->MsgPort ) ))
	{
		unit->ReadPending = FALSE;
		read = unit->ReadActive;

		if ( ! read )
		{
			continue;
		}

		if ( read->io_Flags & VIDEO_IO_ABORT )
		{
			Video_Read_Reply( unit, IOERR_ABORTED );
			continue;
		}

		if ( ( ! unit->StreamAlt ) && ( ioreq->io_Error == USB2Err_Host_Stall ) )
		{
			if ( ! USB2_EPRes_Destall( unit->Stream ) )
			{
				Video_Read_Reply( unit, IOERR_UNITBUSY );
			}

			unit->FrameStarted = FALSE;
			unit->FrameDiscard = TRUE;
			unit->FrameUsed = 0;
			continue;
		}

		if ( ioreq->io_Error != USB2Err_NoError )
		{
			Video_Read_Reply( unit, IOERR_UNITBUSY );
			continue;
		}

		if ( unit->StreamAlt )
		{
			if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length < unit->StreamPacketSize * VIDEO_ISO_PACKETS ) )
			{
				Video_Read_Reply( unit, IOERR_BADLENGTH );
				continue;
			}

			for ( index = 0; ( index < VIDEO_ISO_PACKETS ) && ( unit->ReadActive ); index++ )
			{
				if ( unit->IsoPacketLengths[index] == USB2_ISO_PACKET_ERROR )
				{
					Video_Read_Payload( unit, NULL, 1 );
					continue;
				}

				if ( unit->IsoPacketLengths[index] > unit->StreamPacketSize )
				{
					Video_Read_Reply( unit, IOERR_BADLENGTH );
					break;
				}

				Video_Read_Payload( unit, (U8 *) ioreq->io_Data + index * unit->StreamPacketSize,
					unit->IsoPacketLengths[index] );
			}
		}
		else if ( ioreq->io_Actual <= ioreq->io_Length )
		{
			Video_Read_Payload( unit, ioreq->io_Data, ioreq->io_Actual );
		}
		else
		{
			Video_Read_Reply( unit, IOERR_BADLENGTH );
		}
	}

	Video_Read_Start( unit );
}

// --
