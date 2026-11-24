
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "Firmware.h"
#include "Radio.h"

// --

static int
CopyRead( PTR context, struct IOSana2Req *ioreq, const U8 *data )
{
	return ( RTL8188EU_CopyPacket( context, ioreq, ioreq->ios2_Data, (PTR) data, ioreq->ios2_DataLength, TRUE ) );
}

// --

void
RTL8188EU_ReceiveData( struct EthernetUnit *unit, U8 *frame, U32 length )
{
	static const U8 broadcast[6] = { 255, 255, 255, 255, 255, 255 };
	static const U8 snap[6] = { 0xaa, 0xaa, 3, 0, 0, 0 };

	U8 ethernet[RTL8188EU_MTU + 14];
	U32 header;
	U32 type;
	U32 result;
	U8 flags;
	S32 eapol_only;

	if ( ( ! unit->unit_Online && ! ( RTL8188EU_DeferredLink( unit ) && unit->unit_AssocID ) ) || ( length < 32 ) ||
		( ( frame[0] & 0x4f ) != 0x08 ) || ( ( frame[1] & 7 ) != 2 ) ||
		( frame[22] & 15 ) || ( memcmp( frame + 10, unit->unit_BSSID, 6 ) ) )
	{
		return;
	}

	flags = 0;

	if ( ! memcmp( frame + 4, broadcast, sizeof( broadcast ) ) )
	{
		flags = SANA2IOF_BCAST;
	}
	else if ( frame[4] & 1 )
	{
		if ( ( ! Sana2_MulticastMatch( & unit->unit_Multicast, frame + 4 ) ) &&
			( ! Sana2_MulticastMatch( & unit->unit_MulticastRanges, frame + 4 ) ) )
		{
			return;
		}

		flags = SANA2IOF_MCAST;
	}
	else if ( memcmp( frame + 4, unit->unit_StationAddress, 6 ) )
	{
		return;
	}

	header = 24;

	if ( frame[0] & 0x80 )
	{
		if ( frame[24] & 0x80 )
		{
			return;
		}

		header += ( frame[1] & 0x80 ) ? 6 : 2;
	}

	if ( frame[1] & 0x40 )
	{
		if ( ! RTL8188EU_UnprotectData( unit, frame, & length, & header ) )
		{
			return;
		}
	}

	if ( ( length < header + 8 ) || ( length - header - 8 > RTL8188EU_MTU ) ||
		( memcmp( frame + header, snap, sizeof( snap ) ) ) )
	{
		return;
	}

	type = ( (U32) frame[header + 6] << 8 ) | frame[header + 7];
	eapol_only = ( ! unit->unit_Online ) || ( unit->unit_RequireOperState && ! unit->unit_Operational ) ||
		( unit->unit_Encryption != S2ENC_NONE &&
			( ! ( frame[1] & 0x40 ) || ! unit->unit_Keys[0].active ) );

	if ( ( eapol_only ) && ( ( type != RTL8188EU_EAPOL ) || ( flags ) ) )
	{
		return;
	}

	length -= header + 8;

	memcpy( ethernet, frame + 4, 6 );

	memcpy( ethernet + 6, frame + 16, 6 );

	memcpy( ethernet + 12, frame + header + 6, length + 2 );

	unit->unit_PacketsRx++;

	Sana2_UpdateThroughput( & unit->unit_Pending, 0, length + 14 );

	result = Sana2_DeliverReads( & unit->unit_Pending, ethernet, length + 14, type, flags,
		! eapol_only, FALSE, NULL, CopyRead, NULL, unit );

	if ( ! ( result & SANA2_RX_TYPED ) )
	{
		unit->unit_UnknownPackets++;
	}

	if ( result & SANA2_RX_COPY_FAILED )
	{
		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_BUFF );
	}

	Sana2_AccountType( & unit->unit_TrackedTypes, type, length, TRUE,
		( result & ( SANA2_RX_TYPED | SANA2_RX_COPIED ) ) != ( SANA2_RX_TYPED | SANA2_RX_COPIED ) );
}

// --
