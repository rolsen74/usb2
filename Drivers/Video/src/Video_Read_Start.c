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
struct USB2_IORequest *last;

U32 index;
U32 slot;
U32 previous;

	if ( ( unit->Detached ) || ( ! unit->ReadActive ) )
	{
		return;
	}

	for ( ;; )
	{
		slot = unit->ReadTail;
		ioreq = unit->Stream->IORequests[slot];

		if ( ( unit->ReadPending & ( 1U << slot ) ) || ( ioreq == unit->ReadReady ) )
		{
			break;
		}

		previous = ( slot + VIDEO_USB_READS - 1 ) % VIDEO_USB_READS;
		last = unit->Stream->IORequests[previous];

		// Mark the new batch, leaving any retained packets before the gap usable.
		if ( ( ! unit->ReadPending ) ||
			( ( unit->ReadPending & ( 1U << previous ) ) && ( CheckIO( (PTR) last ) ) ) )
		{
			unit->ReadGaps |= 1U << slot;
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
		unit->ReadTail = ( slot + 1 ) % VIDEO_USB_READS;

		SendIO( (PTR) ioreq );
	}
}

// --
