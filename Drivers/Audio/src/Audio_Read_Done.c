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
Audio_Read_Buffered( struct AudioUnit *unit )
{
	struct IOStdReq *read;

	U32 length;

	read = unit->ReadActive;

	if ( ( ! read ) || ( ! unit->BufferedBytes ) )
	{
		return;
	}

	length = MIN( unit->BufferedBytes, read->io_Length - unit->DataUsed );

	CopyMem( (U8 *) unit->Stream->Buffers[0] + unit->BufferOffset,
		(U8 *) read->io_Data + unit->DataUsed, length );

	unit->DataUsed += length;
	unit->BufferOffset += length;
	unit->BufferedBytes -= length;

	if ( unit->DataUsed == read->io_Length )
	{
		Audio_Read_Reply( unit, 0 );
	}
}

// --

SEC_CODE void
Audio_Read_Done( struct AudioUnit *unit )
{
struct USB2_IORequest *ioreq;
struct IOStdReq *read;

U32 framesize;
U32 length;
U32 index;
U32 packet;
U32 missed;
U32 offset;
U32 byte;
U8 *buffer;
S8 error;

	while(( ioreq = (PTR) GetMsg( unit->Stream->MsgPort ) ))
	{
		unit->ReadPending = FALSE;
		read = unit->ReadActive;

		if ( ! read )
		{
			continue;
		}

		if ( read->io_Flags & AUDIO_IO_ABORT )
		{
			Audio_Read_Reply( unit, IOERR_ABORTED );

			continue;
		}

		if ( ioreq->io_Error != USB2Err_NoError )
		{
			AUDIO_TRACE( "unit %lu endpoint $%02lx read failed, USB error %ld, bytes %lu\n",
				unit->UnitNr, (U32) unit->EndPoint, (S32) ioreq->io_Error, ioreq->io_Actual );

			Audio_Read_Reply( unit, ioreq->io_Error );

			continue;
		}

		if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length < unit->PacketBytes * unit->PacketCount ) )
		{
			Audio_Read_Reply( unit, IOERR_BADLENGTH );

			continue;
		}

		framesize = unit->Format.Channels * 2;
		buffer = ioreq->io_Data;
		length = 0;
		missed = 0;
		error = 0;

		for( index = 0; index < unit->PacketCount; index++ )
		{
			packet = unit->IsoPacketLengths[index];

			if ( packet == USB2_ISO_PACKET_ERROR )
			{
				missed++;
				continue;
			}

			if ( ( packet > unit->PacketBytes ) || ( packet % framesize ) )
			{
				error = IOERR_BADLENGTH;
				break;
			}

			if ( packet )
			{
				offset = index * unit->PacketBytes;

				if ( offset >= length + packet )
				{
					CopyMem( buffer + offset, buffer + length, packet );
				}
				else if ( offset != length )
				{
					for( byte = 0; byte < packet; byte++ )
					{
						buffer[length + byte] = buffer[offset + byte];
					}
				}

				length += packet;
			}
		}

		if ( ( ! error ) && ( length != ioreq->io_Actual ) )
		{
			error = IOERR_BADLENGTH;
		}

		if ( error )
		{
			Audio_Read_Reply( unit, error );

			continue;
		}

		if ( ! length )
		{
			unit->EmptyBatches++;
			unit->EmptyFailedPackets += missed;

			if ( unit->EmptyBatches >= AUDIO_EMPTY_BATCH_LIMIT )
			{
				AUDIO_TRACE( "unit %lu endpoint $%02lx no PCM after %lu reads, %lu of %lu USB intervals failed\n",
					unit->UnitNr, (U32) unit->EndPoint, unit->EmptyBatches,
					unit->EmptyFailedPackets, unit->EmptyBatches * unit->PacketCount );

				Audio_Read_Reply( unit, unit->EmptyFailedPackets == unit->EmptyBatches * unit->PacketCount ?
					AUDIO_ERR_NO_PACKETS : AUDIO_ERR_NO_SAMPLES );
			}

			continue;
		}

		unit->EmptyBatches = 0;
		unit->EmptyFailedPackets = 0;
		unit->BufferOffset = 0;
		unit->BufferedBytes = length;

		Audio_Read_Buffered( unit );
	}

	Audio_Read_Start( unit );
}

// --
