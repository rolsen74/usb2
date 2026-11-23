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
	struct AudioReadBatch *batch;

	U32 length;

	while( ( unit->ReadActive ) && ( unit->ReadCount ) )
	{
		read = unit->ReadActive;
		batch = & unit->Reads[unit->ReadHead];

		if ( batch->Pending )
		{
			break;
		}

		if ( batch->Error )
		{
			Audio_Read_Reply( unit, batch->Error );

			Audio_Read_Stop( unit );

			return;
		}

		if ( batch->Missed )
		{
			unit->ReadDiscontinuity = TRUE;
		}

		if ( batch->Bytes )
		{
			batch->Missed = 0;
			unit->EmptyBatches = 0;
			unit->EmptyFailedPackets = 0;
			length = MIN( batch->Bytes, read->io_Length - unit->DataUsed );

			CopyMem( (U8 *) unit->Stream->Buffers[unit->ReadHead] + batch->Offset,
				(U8 *) read->io_Data + unit->DataUsed, length );

			unit->DataUsed += length;
			batch->Offset += length;
			batch->Bytes -= length;

			if ( ( unit->DataUsed == read->io_Length ) && ( Audio_Read_Reply( unit, 0 ) == IOERR_ABORTED ) )
			{
				Audio_Read_Stop( unit );

				return;
			}
		}
		else
		{
			unit->EmptyBatches++;
			unit->EmptyFailedPackets += batch->Missed;

			if ( unit->EmptyBatches >= AUDIO_EMPTY_BATCH_LIMIT )
			{
				AUDIO_TRACE( "unit %lu endpoint $%02lx no PCM after %lu reads, %lu of %lu USB intervals failed\n",
					unit->UnitNr, (U32) unit->EndPoint, unit->EmptyBatches,
					unit->EmptyFailedPackets, unit->EmptyBatches * unit->PacketCount );

				Audio_Read_Reply( unit, ( unit->EmptyFailedPackets == unit->EmptyBatches * unit->PacketCount ) ?
					AUDIO_ERR_NO_PACKETS : AUDIO_ERR_NO_SAMPLES );

				Audio_Read_Stop( unit );

				return;
			}
		}

		if ( ! batch->Bytes )
		{
			unit->ReadHead = ( unit->ReadHead + 1 ) % unit->ReadSlots;
			unit->ReadCount--;
		}
	}
}

// --

static void
Audio_Read_Complete( struct AudioUnit *unit, struct USB2_IORequest *ioreq, struct AudioReadBatch *batch )
{
	U32 framesize;
	U32 index;
	U32 packet;
	U32 offset;
	U32 byte;
	U8 *buffer;

	batch->Pending = FALSE;
	batch->Error = ioreq->io_Error;
	batch->Bytes = 0;
	batch->Offset = 0;
	batch->Missed = 0;

	if ( batch->Error )
	{
		AUDIO_TRACE( "unit %lu endpoint $%02lx read failed, USB error %ld, bytes %lu\n",
			unit->UnitNr, (U32) unit->EndPoint, (S32) batch->Error, ioreq->io_Actual );

		return;
	}

	if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length < unit->PacketBytes * unit->PacketCount ) )
	{
		batch->Error = IOERR_BADLENGTH;

		return;
	}

	framesize = unit->Format.Channels * 2;
	buffer = ioreq->io_Data;

	for( index = 0; index < unit->PacketCount; index++ )
	{
		packet = batch->PacketLengths[index];

		if ( packet == USB2_ISO_PACKET_ERROR )
		{
			batch->Missed++;

			continue;
		}

		if ( ( packet > unit->PacketBytes ) || ( packet % framesize ) )
		{
			batch->Error = IOERR_BADLENGTH;

			return;
		}

		if ( packet )
		{
			offset = index * unit->PacketBytes;

			if ( offset >= batch->Bytes + packet )
			{
				CopyMem( buffer + offset, buffer + batch->Bytes, packet );
			}
			else if ( offset != batch->Bytes )
			{
				for( byte = 0; byte < packet; byte++ )
				{
					buffer[batch->Bytes + byte] = buffer[offset + byte];
				}
			}

			batch->Bytes += packet;
		}
	}

	if ( batch->Bytes != ioreq->io_Actual )
	{
		batch->Error = IOERR_BADLENGTH;
	}
}

// --

SEC_CODE void
Audio_Read_Done( struct AudioUnit *unit )
{
	struct USB2_IORequest *ioreq;

	U32 slot;

	while(( ioreq = (PTR) GetMsg( unit->Stream->MsgPort ) ))
	{
		slot = ( ioreq == unit->Stream->IORequests[0] ) ? 0 : 1;

		Audio_Read_Complete( unit, ioreq, & unit->Reads[slot] );
	}

	Audio_Read_Buffered( unit );

	Audio_Read_Start( unit );
}

// --
