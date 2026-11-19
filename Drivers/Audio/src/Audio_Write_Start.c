
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
	struct IOStdReq *write;

	U64 samples;
	U32 framesize;
	U32 slot;
	U32 index;
	U32 bytes;
	U32 offset;

	write = unit->ReadActive;

	if ( ( unit->Detached ) || ( ! write ) || ( write->io_Flags & AUDIO_IO_ABORT ) )
	{
		return;
	}

	framesize = unit->Format.Channels * 2;

	for ( slot = 0; ( slot < AUDIO_WRITE_REQUESTS ) && ( unit->WriteQueued < write->io_Length ); slot++ )
	{
		if ( unit->WriteBytes[slot] )
		{
			continue;
		}

		ioreq = unit->Stream->IORequests[slot];
		offset = unit->WriteQueued;

		ioreq->io_Command = CMD_WRITE;
		ioreq->io_Data = unit->Stream->Buffers[slot];
		ioreq->io_AllowShortPackets = FALSE;
		ioreq->io_IsoPacketLengths = unit->WritePacketLengths[slot];
		ioreq->io_TimeOut = AUDIO_READ_TIMEOUT;

		for( index = 0; ( index < unit->PacketCount ) &&
			( unit->WriteQueued < write->io_Length ); index++ )
		{
			samples = (U64) unit->Format.SampleRate * unit->IntervalUS + unit->WritePhase;
			bytes = ( samples / 1000000U ) * framesize;
			unit->WritePhase = samples % 1000000U;
			bytes = MIN( bytes, write->io_Length - unit->WriteQueued );
			unit->WritePacketLengths[slot][index] = bytes;

			CopyMem( (U8 *) write->io_Data + unit->WriteQueued,
				(U8 *) ioreq->io_Data + index * unit->PacketBytes, bytes );

			unit->WriteQueued += bytes;
		}

		ioreq->io_Length = index * unit->PacketBytes;
		ioreq->io_IsoPacketCount = index;
		unit->WriteBytes[slot] = unit->WriteQueued - offset;

		SendIO( (PTR) ioreq );
	}
}

// --
