
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

static S32
CompleteRead( struct EthernetUnit *unit, struct IOSana2Req *ioreq, const U8 *frame,
	const U8 *payload, U32 length, U16 type, U8 flags, struct Hook *filter )
{
	static const U8 empty[SANA2_MAX_ADDR_BYTES] = { 0 };

	U8 ethernet[RTL8821CU_MTU + RTL8821CU_ETH_HEADER];
	U8 oldDestination[SANA2_MAX_ADDR_BYTES];
	U8 oldSource[SANA2_MAX_ADDR_BYTES];
	U32 oldLength;
	U8 oldFlags;
	S32 copied;

	oldLength = ioreq->ios2_DataLength;
	oldFlags = ioreq->ios2_Req.io_Flags;

	memcpy( oldDestination, ioreq->ios2_DstAddr, sizeof( oldDestination ) );

	memcpy( oldSource, ioreq->ios2_SrcAddr, sizeof( oldSource ) );

	CopyMem( (PTR) empty, ioreq->ios2_DstAddr, sizeof( empty ) );

	CopyMem( (PTR) empty, ioreq->ios2_SrcAddr, sizeof( empty ) );

	CopyMem( (PTR) ( frame + 4 ), ioreq->ios2_DstAddr, RTL8821CU_ADDR_SIZE );

	CopyMem( (PTR) ( frame + 16 ), ioreq->ios2_SrcAddr, RTL8821CU_ADDR_SIZE );

	if ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW )
	{
		CopyMem( ioreq->ios2_DstAddr, ethernet, RTL8821CU_ADDR_SIZE );

		CopyMem( ioreq->ios2_SrcAddr, ethernet + RTL8821CU_ADDR_SIZE, RTL8821CU_ADDR_SIZE );

		ethernet[12] = type >> 8;
		ethernet[13] = type;

		CopyMem( (PTR) payload, ethernet + RTL8821CU_ETH_HEADER, length );

		payload = ethernet;
		length += RTL8821CU_ETH_HEADER;
	}
	else if ( ioreq->ios2_Req.io_Command == S2_READORPHAN )
	{
		payload -= 2;
		length += 2;
	}

	ioreq->ios2_DataLength = length;
	ioreq->ios2_Req.io_Flags = ( ioreq->ios2_Req.io_Flags & ~( SANA2IOF_BCAST | SANA2IOF_MCAST ) ) | flags;

	if ( ( filter ) && ( ! CallHookPkt( filter, ioreq, (PTR) payload ) ) )
	{
		ioreq->ios2_DataLength = oldLength;
		ioreq->ios2_Req.io_Flags = oldFlags;

		memcpy( ioreq->ios2_DstAddr, oldDestination, sizeof( oldDestination ) );

		memcpy( ioreq->ios2_SrcAddr, oldSource, sizeof( oldSource ) );

		return ( -1 );
	}

	Remove( (PTR) ioreq );

	ioreq->ios2_PacketType = type;

	copied = RTL8821CU_CopyPacket( ioreq, ioreq->ios2_Data, (PTR) payload, length, TRUE );

	if ( ! copied )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		ioreq->ios2_WireError = S2WERR_BUFF_ERROR;
		ioreq->ios2_DataLength = 0;

		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_BUFF );
	}

	ReplyMsg( (PTR) ioreq );

	return ( copied );
}

// --

static void
CacheEAPOL( struct EthernetUnit *unit, const U8 *frame, const U8 *payload, U32 length )
{
	if ( ( ! unit->unit_Handshake ) || ( length < 4 ) || ( length > sizeof( unit->unit_EAPOL ) ) ||
		( ! RTL8821CU_SameAddress( frame + 16, unit->unit_BSSID ) ) )
	{
		return;
	}

	memcpy( unit->unit_EAPOLHeader, frame, sizeof( unit->unit_EAPOLHeader ) );

	memcpy( unit->unit_EAPOL, payload, length );

	unit->unit_EAPOLLength = length;

	DebugPrintF( "RTL8821CU: queued early EAPOL RX (%lu bytes)\n", length );
}

// --

void
RTL8821CU_DeliverEAPOL( struct EthernetUnit *unit )
{
	struct IOSana2Req *ioreq;
	struct BufferManagement *opener;

	U32 length;
	S32 copied;

	length = unit->unit_EAPOLLength;

	if ( ( ! length ) || ( ! unit->unit_AssocID ) || ( ! unit->unit_Handshake ) )
	{
		return;
	}

	for( ioreq = (PTR) GetHead( & unit->unit_Pending ); ioreq; ioreq = (PTR) GetSucc( (PTR) ioreq ) )
	{
		if ( ( ioreq->ios2_Req.io_Command != CMD_READ ) || ( ioreq->ios2_PacketType != RTL8821CU_EAPOL ) ||
			( ! RTL8821CU_DataAllowed( unit, ioreq ) ) )
		{
			continue;
		}

		opener = ioreq->ios2_BufferManagement;

		copied = CompleteRead( unit, ioreq, unit->unit_EAPOLHeader, unit->unit_EAPOL, length,
			RTL8821CU_EAPOL, 0, opener->bm_PacketHook );

		if ( copied >= 0 )
		{
			unit->unit_EAPOLLength = 0;

			DebugPrintF( "RTL8821CU: delivered early EAPOL RX (%lu bytes)\n", length );

			return;
		}
	}
}

// --

void
RTL8821CU_ReceiveData( struct EthernetUnit *unit, U8 *frame, U32 length )
{
	struct BufferManagement *opener;
	struct IOSana2Req *ioreq;
	struct IOSana2Req *orphan;

	static const U8 broadcast[RTL8821CU_ADDR_SIZE] = { 255, 255, 255, 255, 255, 255 };

	const U8 *snap;
	U32 header;
	U16 type;
	U8 flags;
	S32 delivered;
	S32 copied;
	S32 protected;
	S32 eapol_only;

	header = RTL8821CU_WLAN_HEADER;
	flags = 0;
	orphan = NULL;
	delivered = FALSE;

	if ( ( ( ! unit->unit_AssocID ) || ( unit->unit_ManualOffline ) ) && ( ! unit->unit_Handshake ) )
	{
		return;
	}

	if ( ( length < header + RTL8821CU_LLC_HEADER ) || ( ( frame[0] & 0x7f ) != 0x08 ) ||
		( ( frame[1] & 7 ) != 2 ) || ( frame[22] & 0x0f ) ||
		( ! RTL8821CU_SameAddress( frame + 10, unit->unit_BSSID ) ) )
	{
		return;
	}

	if ( frame[0] & 0x80 )
	{
		if ( frame[24] & 0x80 )
		{
			return; // A-MSDU requires separate subframe processing.
		}

		header += ( frame[1] & 0x80 ) ? 6 : 2;
	}

	protected = ( frame[1] & 0x40 ) != 0;

	if ( ( protected ) && ( ! RTL8821CU_UnprotectData( unit, frame, & length, header ) ) )
	{
		return;
	}

	if ( ( length < header + RTL8821CU_LLC_HEADER ) || ( length - header - RTL8821CU_LLC_HEADER > RTL8821CU_MTU ) )
	{
		return;
	}

	snap = frame + header;
	type = ( (U16) snap[6] << 8 ) | snap[7];

	if ( ( snap[0] != 0xaa ) || ( snap[1] != 0xaa ) || ( snap[2] != 0x03 ) ||
		( snap[3] ) || ( snap[4] ) || ( ( snap[5] != 0 ) && ( snap[5] != 0xf8 ) ) || ( type < 0x0600 ) )
	{
		return;
	}

	if ( RTL8821CU_SameAddress( frame + 4, broadcast ) )
	{
		flags = SANA2IOF_BCAST;
	}
	else if ( frame[4] & 1 )
	{
		if ( ! RTL8821CU_HasMulticast( unit, frame + 4 ) )
		{
			return;
		}

		flags = SANA2IOF_MCAST;
	}
	else if ( ! RTL8821CU_SameAddress( frame + 4, unit->unit_StationAddress ) )
	{
		return;
	}

	eapol_only = ( ! unit->unit_Online ) || ( unit->unit_ManualOffline ) ||
		( ( unit->unit_Encryption == S2ENC_CCMP ) && ( ! protected ) );

	if ( ( eapol_only ) && ( ( type != RTL8821CU_EAPOL ) || ( flags ) ) )
	{
		return;
	}

	length -= header + RTL8821CU_LLC_HEADER;

	if ( ! unit->unit_AssocID )
	{
		if ( ( ! protected ) && ( type == RTL8821CU_EAPOL ) )
		{
			CacheEAPOL( unit, frame, snap + RTL8821CU_LLC_HEADER, length );
		}

		return;
	}

	unit->unit_PacketsRx++;

	Sana2_UpdateThroughput( & unit->unit_Pending, 0, length + RTL8821CU_ETH_HEADER );

	for( opener = (PTR) GetHead( & unit->unit_Openers ); opener; opener = (PTR) GetSucc( & opener->bm_Node ) )
	{
		copied = FALSE;

		for( ioreq = (PTR) GetHead( & unit->unit_Pending ); ioreq; ioreq = (PTR) GetSucc( (PTR) ioreq ) )
		{
			if ( ioreq->ios2_BufferManagement != opener )
			{
				continue;
			}

			if ( ( ioreq->ios2_Req.io_Command == CMD_READ ) && ( ioreq->ios2_PacketType == type ) )
			{
				copied = CompleteRead( unit, ioreq, frame, snap + RTL8821CU_LLC_HEADER,
					length, type, flags, opener->bm_PacketHook );

				if ( copied >= 0 )
				{
					delivered = TRUE;

					break;
				}

				copied = FALSE;
			}

			if ( ( ! eapol_only ) && ( ! orphan ) && ( ioreq->ios2_Req.io_Command == S2_READORPHAN ) )
			{
				orphan = ioreq;
			}
		}

		Sana2_AccountType( & opener->bm_TrackedTypes, type, length, TRUE, ! copied );
	}

	if ( ( type == RTL8821CU_EAPOL ) && ( unit->unit_Handshake ) )
	{
		if ( delivered )
		{
			unit->unit_EAPOLLength = 0;

			DebugPrintF( "RTL8821CU: EAPOL RX %lu bytes\n", length );
		}
		else if ( ( ! protected ) && ( ! flags ) )
		{
			CacheEAPOL( unit, frame, snap + RTL8821CU_LLC_HEADER, length );
		}
	}

	if ( ! delivered )
	{
		unit->unit_UnknownPackets++;

		if ( orphan )
		{
			CompleteRead( unit, orphan, frame, snap + RTL8821CU_LLC_HEADER, length, type, flags, NULL );
		}
	}
}

// --
