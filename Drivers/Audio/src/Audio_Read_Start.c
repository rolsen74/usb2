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
Audio_Read_Start( struct AudioUnit *unit )
{
	struct USB2_IORequest *ioreq;
	struct AudioReadBatch *batch;

	U32 slot;
	U32 index;

	// Keep an established pipeline fed while the app handles the last reply.
	// Completed batches occupy their slots until the app takes the samples.
	while( ( ! unit->Detached ) && ( ! unit->ResetStream ) && ( unit->ReadCount < unit->ReadSlots ) &&
		( unit->ReadActive ? ( ! ( unit->ReadActive->io_Flags & AUDIO_IO_ABORT ) ) : ( unit->ReadCount != 0 ) ) )
	{
		slot = ( unit->ReadHead + unit->ReadCount ) % unit->ReadSlots;
		batch = & unit->Reads[slot];
		ioreq = unit->Stream->IORequests[slot];
		ioreq->io_Command = CMD_READ;
		ioreq->io_Data = unit->Stream->Buffers[slot];
		ioreq->io_Length = unit->PacketBytes * unit->PacketCount;
		ioreq->io_AllowShortPackets = TRUE;
		ioreq->io_IsoPacketLengths = batch->PacketLengths;
		ioreq->io_IsoPacketCount = unit->PacketCount;
		ioreq->io_TimeOut = AUDIO_READ_TIMEOUT;
		ioreq->io_UserValue = slot;

		for( index = 0; index < unit->PacketCount; index++ )
		{
			batch->PacketLengths[index] = USB2_ISO_PACKET_ERROR;
		}

		batch->Pending = TRUE;
		unit->ReadCount++;

		SendIO( (PTR) ioreq );
	}
}

// --
