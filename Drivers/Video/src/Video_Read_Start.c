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

U32 index;
U32 slot;

	if ( ( unit->Detached ) || ( ! unit->ReadActive ) )
	{
		return;
	}

	for ( slot = 0; slot < VIDEO_USB_READS; slot++ )
	{
		ioreq = unit->Stream->IORequests[slot];

		if ( ( unit->ReadPending & ( 1U << slot ) ) || ( ioreq == unit->ReadReady ) )
		{
			continue;
		}

		ioreq->io_Command = CMD_READ;
		ioreq->io_Data = unit->Stream->Buffers[slot];
		ioreq->io_Length = unit->PayloadSize;
		ioreq->io_AllowShortPackets = TRUE;
		ioreq->io_TimeOut = 5 * 1000000;
		ioreq->io_UserValue = slot;

		if ( unit->StreamAlt )
		{
			ioreq->io_Length = unit->StreamPacketSize * unit->IsoPacketCount;
			ioreq->io_IsoPacketLengths = unit->IsoPacketLengths[slot];
			ioreq->io_IsoPacketCount = unit->IsoPacketCount;

			for ( index = 0; index < unit->IsoPacketCount; index++ )
			{
				unit->IsoPacketLengths[slot][index] = USB2_ISO_PACKET_ERROR;
			}
		}

		unit->ReadPending |= 1U << slot;

		SendIO( (PTR) ioreq );
	}
}

// --
