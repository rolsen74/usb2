
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void
DeliverPacket( struct EthernetUnit *unit, const U8 *frame, U8 *body, U32 length )
{
	struct IOSana2Req *request;
	struct IOSana2Req *orphan;

	static const U8 broadcast[] = { 255, 255, 255, 255, 255, 255 };
	U8 ethernet[RT2870_MTU + 14];
	U8 *payload;
	U32 type;
	U32 dhcp;
	U8 flags;
	U32 payload_length;
	S32 typed;

	type = ( (U32) body[6] << 8 ) | body[7];
	payload = body + 8;
	flags = 0;
	dhcp = 0;
	orphan = NULL;
	payload_length = length;

	if ( ! memcmp( frame + 4, broadcast, sizeof( broadcast ) ) )
	{
		flags = SANA2IOF_BCAST;
	}
	else if ( frame[4] & 1 )
	{
		flags = SANA2IOF_MCAST;
	}

	if ( type == 0x0800 )
	{
		dhcp = RT2870_DHCPType( payload, length, unit->unit_StationAddress, TRUE );
	}

	ObtainSemaphore( & unit->unit_CommandLock );

	if ( ( flags == SANA2IOF_MCAST ) && ( ! Sana2_MulticastMatch( & unit->unit_MulticastRanges, frame + 4 ) ) )
	{
		ReleaseSemaphore( & unit->unit_CommandLock );

		return;
	}

	unit->unit_PacketsRx++;

	Sana2_UpdateThroughput( & unit->unit_Pending, 0, length + 14 );

	request = (PTR) GetHead( & unit->unit_Pending );

	while( request )
	{
		if ( ( request->ios2_Req.io_Command == CMD_READ ) && ( request->ios2_PacketType == type ) )
		{
			break;
		}

		if ( ( ! orphan ) && ( request->ios2_Req.io_Command == S2_READORPHAN ) )
		{
			orphan = request;
		}

		request = (PTR) GetSucc( (PTR) request );
	}

	typed = request != NULL;

	if ( ! typed )
	{
		unit->unit_UnknownPackets++;
		request = orphan;

		Sana2_AccountType( & unit->unit_TrackedTypes, type, payload_length, TRUE, TRUE );
	}

	if ( request )
	{
		Remove( (PTR) request );

		memcpy( request->ios2_DstAddr, frame + 4, 6 );

		memcpy( request->ios2_SrcAddr, frame + 16, 6 );

		if ( request->ios2_Req.io_Flags & SANA2IOF_RAW )
		{
			memcpy( ethernet, frame + 4, 6 );

			memcpy( ethernet + 6, frame + 16, 6 );

			memcpy( ethernet + 12, body + 6, 2 );

			memcpy( ethernet + 14, payload, length );

			payload = ethernet;
			length += 14;
		}

		else if ( ! typed )
		{
			payload -= 2;
			length += 2;
		}

		request->ios2_PacketType = type;
		request->ios2_Req.io_Flags &= ~( SANA2IOF_MCAST | SANA2IOF_BCAST );
		request->ios2_Req.io_Flags |= flags;
		request->ios2_DataLength = length;

		if ( ! RT2870_CopyPacket( request, request->ios2_Data, payload, length, TRUE ) )
		{
			request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
			request->ios2_WireError = S2WERR_BUFF_ERROR;
			request->ios2_DataLength = 0;

			if ( typed )
			{
				Sana2_AccountType( & unit->unit_TrackedTypes, type, payload_length, TRUE, TRUE );
			}

			RT2870_ReportDHCP( & unit->unit_DHCPDropped, dhcp, "receive buffer copy failed" );

			Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_BUFF, FALSE );
		}
		else
		{
			if ( typed )
			{
				Sana2_AccountType( & unit->unit_TrackedTypes, type, payload_length, TRUE, FALSE );
			}

			RT2870_ReportDHCP( & unit->unit_DHCPDelivered, dhcp, "delivered to TCP/IP" );
		}

		ReplyMsg( (PTR) request );
	}
	else
	{
		RT2870_ReportDHCP( & unit->unit_DHCPDropped, dhcp, "received with no pending IPv4 read" );
	}

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --

void
RT2870_ReceiveData( struct EthernetUnit *unit, U8 *frame, U32 length, U32 padding )
{
	struct RT2870Key *key;

	static const U8 snap[] = { 0xaa, 0xaa, 3, 0, 0, 0 };
	const U8 *iv;
	U8 *body;
	U32 header;
	U32 tid;
	U32 index;
	U32 size;
	U32 category;
	U32 trailer;
	S32 status;
	U32 i;
	U64 number;

	if ( ( length < 24 ) || ( ! unit->unit_Operational && ! ( unit->unit_TKIP && unit->unit_M4Sent ) ) ||
		( ( frame[0] != 0x08 ) && ( frame[0] != 0x88 ) ) ||
		( ( frame[1] & 0xc7 ) != 0x42 ) || ( frame[22] & 0x0f ) ||
		( memcmp( frame + 10, unit->unit_Peer, 6 ) ) ||
		( ( ! ( frame[4] & 1 ) ) && ( memcmp( frame + 4, unit->unit_StationAddress, 6 ) ) ) )
	{
		return;
	}

	header = 24;
	tid = WIRELESS_NONQOS;

	if ( frame[0] == 0x88 )
	{
		header += 2;

		if ( ( length < header ) || ( frame[24] & 0x80 ) )
		{
			return;
		}

		tid = frame[24] & 0xf;
	}

	trailer = unit->unit_TKIP ? WIRELESS_TKIP_MIC + WIRELESS_TKIP_ICV : WIRELESS_CCMP_MIC;

	if ( length < header + WIRELESS_CCMP_IV + sizeof( snap ) + 2 + trailer )
	{
		return;
	}

	size = length - header - WIRELESS_CCMP_IV - trailer;

	if ( size > RT2870_MTU + sizeof( snap ) + 2 )
	{
		return;
	}

	iv = frame + header + padding;
	index = iv[3] >> 6;

	if ( ( frame[4] & 1 ) && ( ! index ) && ( unit->unit_TKIP ) )
	{
		index = USB2WIRELESS_TKIP_GROUP_ZERO;
	}

	if ( ( ( iv[3] & 0x3f ) != 0x20 ) ||
		( ( index != 0 ) != ( ( frame[4] & 1 ) != 0 ) ) ||
		( ! ( unit->unit_ActiveKeys & ( 1U << index ) ) ) )
	{
		return;
	}

	key = & unit->unit_Keys[index];

	if ( unit->unit_TKIP )
	{
		if ( iv[1] != ( ( iv[0] | 0x20 ) & 0x7f ) )
		{
			return;
		}

		number = iv[2] | ( (U64) iv[0] << 8 );
	}
	else
	{
		if ( iv[2] )
		{
			return;
		}

		number = iv[0] | ( (U64) iv[1] << 8 );
	}

	for( i = 2; i < 6; i++ )
	{
		number |= (U64) iv[i + 2] << ( 8 * i );
	}

	body = frame + header + padding + WIRELESS_CCMP_IV;

	if ( number <= key->rx_number[tid] )
	{
		return;
	}

	if ( unit->unit_TKIP )
	{
		status = Wireless_TKIP( key->tkip_key, frame, tid, number, body, size, FALSE );

		if ( ( status <= 0 ) && ( ! unit->unit_Operational ) && ( unit->unit_TKIPRxFailures < 4 ) )
		{
			unit->unit_TKIPRxFailures++;

			USBINFO( "RT2870: WPA protected RX failed TKIP %s verification", status < 0 ? "Michael MIC" : "ICV" );
		}

		if ( status < 0 )
		{
			RT2870_TKIPFailure( unit );

			return;
		}
	}
	else
	{
		status = Wireless_CCMP( key->round_keys, frame, tid, number, body, size, body + size, FALSE );
	}

	if ( ! status )
	{
		return;
	}

	key->rx_number[tid] = number;

	if ( memcmp( body, snap, sizeof( snap ) ) )
	{
		return;
	}

	if ( ( body[6] == 0x88 ) && ( body[7] == 0x8e ) )
	{
		if ( ( ! index ) && ( ! memcmp( frame + 16, unit->unit_Peer, 6 ) ) )
		{
			RT2870_CacheEAPOLBody( unit, body + 8, size - 8 );
		}
	}
	else if ( unit->unit_Online )
	{
		category = index ? 2 : 1;

		if ( ! ( unit->unit_DataReceived & category ) )
		{
			unit->unit_DataReceived |= category;

			USBINFO( "RT2870: first encrypted %s packet received, EtherType $%04lx",
				index ? "group" : "unicast", ( (U32) body[6] << 8 ) | body[7] );
		}

		DeliverPacket( unit, frame, body, size - 8 );
	}
}

// --
