
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static int
FilterRead( struct IOSana2Req *ioreq, const U8 *data )
{
	struct BufferManagement *opener;

	opener = ioreq->ios2_BufferManagement;

	return ( ( ! opener ) || ( ! opener->bm_PacketHook ) ||
		( IUtility->CallHookPkt( opener->bm_PacketHook, ioreq, (PTR) data ) ) );
}

// --

static int
CopyRead( PTR context, struct IOSana2Req *ioreq, const U8 *data )
{
	return ( Copy_To_Buffer( context, ioreq, (PTR) data ) );
}

// --

void
ReadPacket( struct devUnit *unit, U8 *data, U32 length )
{
	U32 packet_type;
	U32 result;
	U8 flags;

	packet_type = ( (U32) data[12] << 8 ) | data[13];
	flags = 0;
	unit->unit_PacketsReceived++;

	UpdateThroughput( unit, 0, length );

	if ( IS_BROADCAST( data ) )
	{
		flags = SANA2IOF_BCAST;
	}
	else if ( IS_MULTICAST( data ) )
	{
		if ( ( ! Sana2_MulticastMatch( & unit->unit_MultiCastList, data ) ) &&
			( ! Sana2_MulticastMatch( & unit->unit_MulticastRanges, data ) ) )
		{
			unit->unit_MultiCastRejects++;

			return;
		}

		flags = SANA2IOF_MCAST;
	}

	result = Sana2_DeliverReads( & unit->unit_RxList, data, length, packet_type, flags,
		TRUE, TRUE, FilterRead, CopyRead, ReplySanaRequest, unit );

#ifdef WIRED_TRACE_RX_DELIVERY
	WIRED_TRACE_RX_DELIVERY( unit, packet_type, result );
#endif

	if ( ! ( result & SANA2_RX_TYPED ) )
	{
		unit->unit_UnknownTypesReceived++;
	}

	if ( result & SANA2_RX_COPY_FAILED )
	{
		SignalSanaEvent( unit, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_BUFF );
	}

	Sana2_AccountType( & unit->unit_TrackedTypes, packet_type, length - 14, TRUE,
		( result & ( SANA2_RX_TYPED | SANA2_RX_COPIED ) ) != ( SANA2_RX_TYPED | SANA2_RX_COPIED ) );
}

// --
