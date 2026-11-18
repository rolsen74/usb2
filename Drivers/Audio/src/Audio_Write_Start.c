
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
	U32 index;
	U32 bytes;

	write = unit->ReadActive;

	if ( ( unit->Detached ) || ( unit->ReadPending ) || ( ! write ) )
	{
		return;
	}

	ioreq = unit->Stream->IORequests[0];
	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Data = unit->Stream->Buffers[0];
	ioreq->io_AllowShortPackets = FALSE;
	ioreq->io_IsoPacketLengths = unit->IsoPacketLengths;
	ioreq->io_TimeOut = AUDIO_READ_TIMEOUT;
	framesize = unit->Format.Channels * 2;
	unit->WriteBytes = 0;

	for( index = 0; ( index < unit->PacketCount ) &&
		( unit->DataUsed + unit->WriteBytes < write->io_Length ); index++ )
	{
		samples = (U64) unit->Format.SampleRate * unit->IntervalUS + unit->WritePhase;
		bytes = ( samples / 1000000U ) * framesize;
		unit->WritePhase = samples % 1000000U;
		bytes = MIN( bytes, write->io_Length - unit->DataUsed - unit->WriteBytes );
		unit->IsoPacketLengths[index] = bytes;

		CopyMem( (U8 *) write->io_Data + unit->DataUsed + unit->WriteBytes,
			(U8 *) ioreq->io_Data + index * unit->PacketBytes, bytes );

		unit->WriteBytes += bytes;
	}

	ioreq->io_Length = index * unit->PacketBytes;
	ioreq->io_IsoPacketCount = index;
	unit->ReadPending = TRUE;

	SendIO( (PTR) ioreq );
}

// --
