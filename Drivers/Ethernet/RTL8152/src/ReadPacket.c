
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
ReadPacket( struct devUnit *unit, U8 *data, U32 length )
{
	struct IOSana2Req *ioreq;
	struct IOSana2Req *orphan;
	struct BufferManagement *bm;

	U32 packet_type;
	U32 flags;
	U32 header;
	U32 old_length;
	U8 old_flags;

	packet_type = ( (U32) data[12] << 8 ) | data[13];
	flags = 0;
	orphan = NULL;
	header = 0;
	unit->unit_PacketsReceived++;

	UpdateThroughput( unit, 0, length );

	if ( IS_BROADCAST( data ) )
	{
		flags = SANA2IOF_BCAST;
	}
	else if ( IS_MULTICAST( data ) )
	{
		if ( ! FindMulticastAddress( unit, data ) )
		{
			unit->unit_MultiCastRejects++;

			return;
		}

		flags = SANA2IOF_MCAST;
	}

	ioreq = (PTR) IExec->GetHead( & unit->unit_RxList );

	while( ioreq )
	{
		if ( ioreq->ios2_Req.io_Command == S2_READORPHAN )
		{
			if ( ! orphan )
			{
				orphan = ioreq;
			}
		}
		else if ( ioreq->ios2_PacketType == packet_type )
		{
			header = ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW ) ? 0 : 14;
			bm = ioreq->ios2_BufferManagement;
			old_length = ioreq->ios2_DataLength;
			old_flags = ioreq->ios2_Req.io_Flags;
			ioreq->ios2_DataLength = length - header;
			ioreq->ios2_Req.io_Flags = ( old_flags & ~( SANA2IOF_MCAST | SANA2IOF_BCAST ) ) | flags;

			memcpy( ioreq->ios2_DstAddr, data, 6 );

			memcpy( ioreq->ios2_SrcAddr, data + 6, 6 );

			if ( ( ! bm ) || ( ! bm->bm_PacketHook ) ||
				( IUtility->CallHookPkt( bm->bm_PacketHook, ioreq, data + header ) ) )
			{
				break;
			}

			ioreq->ios2_DataLength = old_length;
			ioreq->ios2_Req.io_Flags = old_flags;
		}

		ioreq = (PTR) IExec->GetSucc( (PTR) ioreq );
	}

	if ( ! ioreq )
	{
		unit->unit_UnknownTypesReceived++;

		if ( ! orphan )
		{
			return;
		}

		ioreq = orphan;

		// Non-raw orphan replies include the Ethernet type before the payload.
		header = ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW ) ? 0 : 12;
		ioreq->ios2_DataLength = length - header;
		ioreq->ios2_Req.io_Flags &= ~( SANA2IOF_MCAST | SANA2IOF_BCAST );
		ioreq->ios2_Req.io_Flags |= flags;

		memcpy( ioreq->ios2_DstAddr, data, 6 );

		memcpy( ioreq->ios2_SrcAddr, data + 6, 6 );
	}

	ioreq->ios2_PacketType = packet_type;
	ioreq->ios2_WireError = 0;

	IExec->Remove( (PTR) ioreq );

	if ( ! Copy_To_Buffer( unit, ioreq, data + header ) )
	{
		CheckEvent( unit, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_BUFF );

		ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		ioreq->ios2_WireError = S2WERR_BUFF_ERROR;
	}

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
