/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Video_Read_Start( struct VideoUnit *unit )
{
struct USB2_IORequest *ioreq;

	if ( ( unit->Detached ) || ( unit->ReadPending ) || ( ! unit->ReadActive ) )
	{
		return;
	}

	ioreq = unit->Stream->IORequests[0];
	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = unit->Stream->Buffers[0];
	ioreq->io_Length = unit->PayloadSize;
	ioreq->io_AllowShortPackets = TRUE;

	if ( unit->StreamAlt )
	{
		ioreq->io_Length = unit->StreamPacketSize * VIDEO_ISO_PACKETS;
		ioreq->io_IsoPacketLengths = unit->IsoPacketLengths;
		ioreq->io_IsoPacketCount = VIDEO_ISO_PACKETS;
		ioreq->io_TimeOut = 5 * 1000000;

		for ( U32 index = 0; index < VIDEO_ISO_PACKETS; index++ )
		{
			unit->IsoPacketLengths[index] = 0;
		}
	}

	unit->ReadPending = TRUE;

	SendIO( (PTR) ioreq );
}

// --
