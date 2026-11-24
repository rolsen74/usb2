
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

static int
CopyRead( PTR context, struct IOSana2Req *ioreq, const U8 *data )
{
	return ( RTL8811AU_CopyPacket( context, ioreq, ioreq->ios2_Data, (PTR) data, ioreq->ios2_DataLength, TRUE ) );
}

// --

static S32
MulticastMatch( struct EthernetUnit *unit, const U8 *address )
{
	struct BufferManagement *opener;

	for( opener = (PTR) GetHead( & unit->unit_Openers ); opener; opener = (PTR) GetSucc( & opener->bm_Node ) )
	{
		if ( Sana2_MulticastMatch( & opener->bm_Multicast, address ) )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

void
RTL8811AU_ReceiveData( struct EthernetUnit *unit, U8 *frame, U32 length )
{
	static const U8 broadcast[6] = { 255, 255, 255, 255, 255, 255 };
	static const U8 snap[] = { 0xaa, 0xaa, 3, 0, 0, 0 };

	U8 ethernet[RTL8811AU_MTU + RTL8811AU_ETH_HEADER];
	U32 header;
	U32 type;
	U32 result;
	U8 flags;
	S32 protected;
	S32 eapol_only;

	if ( ( ! unit->unit_AssocID ) || ( unit->unit_ManualOffline ) || ( length < 32 ) ||
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
		if ( ! MulticastMatch( unit, frame + 4 ) )
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

	protected = ( frame[1] & 0x40 ) != 0;

	if ( ( protected ) && ( ! RTL8811AU_UnprotectData( unit, frame, & length, header ) ) )
	{
		return;
	}

	if ( ( length < header + 8 ) || ( length - header - 8 > RTL8811AU_MTU ) ||
		( memcmp( frame + header, snap, sizeof( snap ) ) ) )
	{
		return;
	}

	type = ( (U32) frame[header + 6] << 8 ) | frame[header + 7];
	eapol_only = ( ! unit->unit_Online ) || ( unit->unit_Encryption == S2ENC_CCMP && ! protected );

	if ( ( eapol_only ) && ( ( type != RTL8811AU_EAPOL ) || ( flags ) ) )
	{
		return;
	}

	length -= header + 8;

	memcpy( ethernet, frame + 4, 6 );
	memcpy( ethernet + 6, frame + 16, 6 );
	memcpy( ethernet + 12, frame + header + 6, length + 2 );

	unit->unit_PacketsRx++;

	Sana2_UpdateThroughput( & unit->unit_Pending, 0, length + RTL8811AU_ETH_HEADER );

	result = Sana2_DeliverReads( & unit->unit_Pending, ethernet, length + RTL8811AU_ETH_HEADER,
		type, flags, ! eapol_only, FALSE, NULL, CopyRead, NULL, unit );

	if ( ! ( result & SANA2_RX_TYPED ) )
	{
		unit->unit_UnknownPackets++;
	}

	if ( result & SANA2_RX_COPY_FAILED )
	{
		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_BUFF );
	}

	RTL8811AU_AccountType( unit, type, length, TRUE, ! ( result & SANA2_RX_COPIED ) );
}

// --
