
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

void
RTL8188EU_ReceiveData( struct EthernetUnit *unit, U8 *frame, U32 length )
{
	static const U8 broadcast[6] = { 255, 255, 255, 255, 255, 255 };
	static const U8 snap[6] = { 0xaa, 0xaa, 3, 0, 0, 0 };
	struct IOSana2Req *ioreq;
	struct IOSana2Req *orphan;

	U8 ethernet[RTL8188EU_MTU + 14];
	U8 *payload;
	U32 header;
	U32 type;
	U8 flags;
	S32 eapol_only;

	if ( ( ! unit->unit_Online ) || ( length < 32 ) ||
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
		if ( ! FindMulticastAddress( unit, frame + 4 ) )
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
	eapol_only = ( unit->unit_RequireOperState && ! unit->unit_Operational ) ||
		( unit->unit_Encryption != S2ENC_NONE &&
			( ! ( frame[1] & 0x40 ) || ! unit->unit_Keys[0].active ) );

	if ( ( eapol_only ) && ( ( type != RTL8188EU_EAPOL ) || ( flags ) ) )
	{
		return;
	}

	payload = frame + header + 8;
	length -= header + 8;
	unit->unit_PacketsRx++;

	RTL8188EU_UpdateThroughput( unit, 0, length + 14 );
	orphan = NULL;

	ioreq = (PTR) GetHead( & unit->unit_Pending );

	while( ioreq )
	{
		if ( ( ioreq->ios2_Req.io_Command == CMD_READ ) && ( ioreq->ios2_PacketType == type ) )
		{
			break;
		}

		if ( ( ! eapol_only ) && ( ! orphan ) && ( ioreq->ios2_Req.io_Command == S2_READORPHAN ) )
		{
			orphan = ioreq;
		}

		ioreq = (PTR) GetSucc( (PTR) ioreq );
	}

	if ( ! ioreq )
	{
		unit->unit_UnknownPackets++;
		ioreq = orphan;

		if ( ! ioreq )
		{
			return;
		}
	}

	if ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW )
	{
		memcpy( ethernet, frame + 4, 6 );
		memcpy( ethernet + 6, frame + 16, 6 );
		memcpy( ethernet + 12, frame + header + 6, 2 );
		memcpy( ethernet + 14, payload, length );

		payload = ethernet;
		length += 14;
	}
	else if ( ioreq->ios2_Req.io_Command == S2_READORPHAN )
	{
		payload -= 2;
		length += 2;
	}

	Remove( (PTR) ioreq );

	memcpy( ioreq->ios2_DstAddr, frame + 4, 6 );
	memcpy( ioreq->ios2_SrcAddr, frame + 16, 6 );

	ioreq->ios2_PacketType = type;
	ioreq->ios2_Req.io_Flags = ( ioreq->ios2_Req.io_Flags & ~( SANA2IOF_MCAST | SANA2IOF_BCAST ) ) | flags;
	ioreq->ios2_DataLength = length;

	if ( ! RTL8188EU_CopyPacket( unit, ioreq, ioreq->ios2_Data, payload, length, TRUE ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		ioreq->ios2_WireError = S2WERR_BUFF_ERROR;
		ioreq->ios2_DataLength = 0;

		SignalSana2Event( unit, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_BUFF );
	}

	ReplyMsg( (PTR) ioreq );
}

// --
