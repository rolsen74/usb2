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
	U32 offset;
	S32 consumed;
	S8 error;
	U32 budget;
	U32 slot;

	budget = VIDEO_ISO_MAX_PACKETS;

	while ( ( unit->ReadActive ) && ( budget ) )
	{
		if ( unit->ReadActive->io_Flags & VIDEO_IO_ABORT )
		{
			Video_Read_Stop( unit );

			Video_Read_Reply( unit, IOERR_ABORTED );
			break;
		}

		if ( CheckIO( & unit->Timer->Request ) )
		{
			Video_Read_Reply( unit, VIDEO_ERR_FRAME_TIMEOUT );
			break;
		}

		budget--;
		ioreq = unit->ReadReady;

		if ( ! ioreq )
		{
			slot = unit->ReadHead;

			if ( ! ( unit->ReadPending & ( 1U << slot ) ) )
			{
				break;
			}

			ioreq = unit->Stream->IORequests[slot];

			if ( ! CheckIO( (PTR) ioreq ) )
			{
				break;
			}

			WaitIO( (PTR) ioreq );

			unit->ReadPending &= ~( 1U << slot );
			unit->ReadHead = ( slot + 1 ) % VIDEO_USB_READS;

			if ( unit->ReadGaps & ( 1U << slot ) )
			{
				unit->ReadGaps &= ~( 1U << slot );

				Video_Read_Payload( unit, NULL, 1 );
			}

			if ( ioreq->io_Error )
			{
				error = ioreq->io_Error;

				if ( error == USB2Err_Device_Detached )
				{
					unit->USBDetached = TRUE;
				}

				Video_Read_Stop( unit );

				if ( ( unit->StreamAlt ) || ( error != USB2Err_Host_Stall ) ||
					( ! USB2_EPRes_Destall( unit->Stream ) ) )
				{
					Video_Read_Reply( unit, error );
				}

				break;
			}

			if ( ( ! ioreq->io_Data ) || ( ioreq->io_Actual > ioreq->io_Length ) ||
				( ( unit->StreamAlt ) && ( ioreq->io_Length < unit->StreamPacketSize * unit->IsoPacketCount ) ) )
			{
				DebugPrintF( "USB Video: unit %lu invalid USB read, %lu/%lu bytes, packet size %lu\n",
					unit->UnitNr, ioreq->io_Actual, ioreq->io_Length, unit->StreamPacketSize );

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
				DebugPrintF( "USB Video: unit %lu USB packet %lu, %lu bytes, slot %lu bytes\n",
					unit->UnitNr, unit->ReadPacket, length, unit->StreamPacketSize );

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
		else if ( unit->OV534 )
		{
			// A bulk read can contain several independently headed OV534 payloads.
			count = MAX( 1, ( ioreq->io_Actual + VIDEO_OV534_PAYLOAD_SIZE - 1 ) / VIDEO_OV534_PAYLOAD_SIZE );
			offset = unit->ReadPacket * VIDEO_OV534_PAYLOAD_SIZE;
			length = MIN( ioreq->io_Actual - offset, VIDEO_OV534_PAYLOAD_SIZE );

			consumed = Video_Read_Payload( unit, (U8 *) ioreq->io_Data + offset, length );
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

	if ( ( ! budget ) && ( unit->ReadActive ) )
	{
		Signal( unit->StreamPort->mp_SigTask, 1UL << unit->StreamPort->mp_SigBit );
	}
}

// --
