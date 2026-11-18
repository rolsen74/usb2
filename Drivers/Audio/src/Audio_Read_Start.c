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

U32 index;

	if ( ( unit->Detached ) || ( unit->ReadPending ) || ( ! unit->ReadActive ) )
	{
		return;
	}

	ioreq = unit->Stream->IORequests[0];
	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = unit->Stream->Buffers[0];
	ioreq->io_Length = unit->PacketBytes * unit->PacketCount;
	ioreq->io_AllowShortPackets = TRUE;
	ioreq->io_IsoPacketLengths = unit->IsoPacketLengths;
	ioreq->io_IsoPacketCount = unit->PacketCount;
	ioreq->io_TimeOut = AUDIO_READ_TIMEOUT;

	for( index = 0; index < unit->PacketCount; index++ )
	{
		unit->IsoPacketLengths[index] = USB2_ISO_PACKET_ERROR;
	}

	unit->ReadPending = TRUE;

	SendIO( (PTR) ioreq );
}

// --
