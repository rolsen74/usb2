
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
Audio_Write_Start( struct AudioUnit *unit )
{
	struct USB2_IORequest *ioreq;
	struct AudioWrite *write;

	U64 samples;
	U32 framesize;
	U32 slot;
	U32 index;
	U32 bytes;
	U32 offset;
	U32 request;

	if ( unit->Detached )
	{
		return;
	}

	framesize = unit->Format.Channels * 2;

	for( request = 0; request < unit->WriteCount; request++ )
	{
		write = & unit->Writes[( unit->WriteHead + request ) % AUDIO_WRITE_REQUESTS];

		if ( write->Request->io_Flags & AUDIO_IO_ABORT )
		{
			return;
		}

		while( write->Queued < write->Request->io_Length )
		{
			slot = unit->WriteSlot;

			if ( unit->WriteOwners[slot] )
			{
				return;
			}

			ioreq = unit->Stream->IORequests[slot];
			offset = write->Queued;
			ioreq->io_Command = CMD_WRITE;
			ioreq->io_Actual = 0;
			ioreq->io_Data = unit->Stream->Buffers[slot];
			ioreq->io_AllowShortPackets = FALSE;
			ioreq->io_IsoPacketLengths = unit->WritePacketLengths[slot];
			ioreq->io_TimeOut = AUDIO_READ_TIMEOUT;
			ioreq->io_UserValue = slot;

			for( index = 0; ( index < unit->PacketCount ) &&
				( write->Queued < write->Request->io_Length ); index++ )
			{
				samples = (U64) unit->Format.SampleRate * unit->IntervalUS + unit->WritePhase;
				bytes = ( samples / 1000000U ) * framesize;
				unit->WritePhase = samples % 1000000U;
				bytes = MIN( bytes, write->Request->io_Length - write->Queued );
				unit->WritePacketLengths[slot][index] = bytes;

				CopyMem( (U8 *) write->Request->io_Data + write->Queued,
					(U8 *) ioreq->io_Data + index * unit->PacketBytes, bytes );

				write->Queued += bytes;
			}

			ioreq->io_Length = index * unit->PacketBytes;
			ioreq->io_IsoPacketCount = index;
			unit->WriteBytes[slot] = write->Queued - offset;
			unit->WriteOwners[slot] = write;
			unit->WriteSlot = ( slot + 1 ) % AUDIO_USB_REQUESTS;

			SendIO( (PTR) ioreq );
		}
	}
}

// --
