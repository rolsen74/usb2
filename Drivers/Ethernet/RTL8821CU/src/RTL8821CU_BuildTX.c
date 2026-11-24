
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

#define TX_LAST_SEGMENT 0x04
#define TX_USE_RATE 0x01
#define TX_RATE_CCK_1MBPS 0
#define TX_QUEUE_MANAGEMENT 0x12
#define TX_NO_RATE_FALLBACK 0x04
#define TX_HW_SEQUENCE 0x80

// --

U32
RTL8821CU_BuildTX( struct EthernetUnit *unit, struct IOSana2Req *ioreq, U8 *buffer, U32 *packet_type, U32 *payload_size )
{
	static const U8 empty[RTL8821CU_TX_DESC_SIZE + RTL8821CU_WLAN_HEADER] = { 0 };
	static const U8 snap[] = { 0xaa, 0xaa, 0x03, 0, 0, 0 };

	const U8 *destination;
	U8 ethernet[RTL8821CU_MTU + RTL8821CU_ETH_HEADER];
	U8 *frame;
	U8 *payload;
	U32 length;
	U32 type;
	S32 management;

	frame = buffer + RTL8821CU_TX_DESC_SIZE;
	payload = frame + RTL8821CU_WLAN_HEADER + RTL8821CU_LLC_HEADER;
	length = ioreq->ios2_DataLength;
	type = ioreq->ios2_PacketType;
	destination = ioreq->ios2_DstAddr;
	management = ioreq->ios2_Req.io_Command == S2_WRITEMGMT;
	*packet_type = 0;
	*payload_size = 0;

	if ( management )
	{
		CopyMem( ioreq->ios2_Data, frame, length );

		CopyMem( (PTR) empty, buffer, RTL8821CU_TX_DESC_SIZE );

		buffer[5] = TX_QUEUE_MANAGEMENT;

		goto descriptor;
	}

	if ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW )
	{
		if ( ! RTL8821CU_CopyPacket( ioreq, ethernet, ioreq->ios2_Data, length, FALSE ) )
		{
			goto copy_failed;
		}

		if ( ioreq->ios2_Req.io_Command == CMD_WRITE )
		{
			destination = ethernet;
		}

		type = ( (U32) ethernet[12] << 8 ) | ethernet[13];
		length -= RTL8821CU_ETH_HEADER;

		CopyMem( ethernet + RTL8821CU_ETH_HEADER, payload, length );
	}
	else if ( ! RTL8821CU_CopyPacket( ioreq, payload, ioreq->ios2_Data, length, FALSE ) )
	{
		goto copy_failed;
	}

	if ( ( type < 0x0600 ) || ( type > 0xffff ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;

		return ( 0 );
	}

	CopyMem( (PTR) empty, buffer, sizeof( empty ) );

	frame[0] = 0x08;
	frame[1] = 0x01; // Infrastructure station, To DS.

	CopyMem( unit->unit_BSSID, frame + 4, RTL8821CU_ADDR_SIZE );

	CopyMem( unit->unit_StationAddress, frame + 10, RTL8821CU_ADDR_SIZE );

	CopyMem( (PTR) destination, frame + 16, RTL8821CU_ADDR_SIZE );

	CopyMem( (PTR) snap, frame + RTL8821CU_WLAN_HEADER, sizeof( snap ) );

	if ( ( type == 0x80f3 ) || ( type == 0x8137 ) )
	{
		frame[29] = 0xf8; // Bridge-tunnel SNAP for AARP and IPX.
	}

	frame[30] = type >> 8;
	frame[31] = type;
	*packet_type = type;
	*payload_size = length;
	length += RTL8821CU_WLAN_HEADER + RTL8821CU_LLC_HEADER;

	if ( ! RTL8821CU_ProtectData( unit, frame, & length, type ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;

		return ( 0 );
	}

descriptor:

	// TX gain is programmed for the legacy 1 Mbps CCK rate.
	buffer[0] = length;
	buffer[1] = length >> 8;
	buffer[2] = RTL8821CU_TX_DESC_SIZE;
	buffer[3] = TX_LAST_SEGMENT | ( frame[4] & 1 );
	buffer[13] = TX_USE_RATE | TX_NO_RATE_FALLBACK;
	buffer[16] = TX_RATE_CCK_1MBPS;
	buffer[33] = TX_HW_SEQUENCE;

	RTL8821CU_ChecksumTX( buffer );

	return ( RTL8821CU_TX_DESC_SIZE + length );

copy_failed:

	ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
	ioreq->ios2_WireError = S2WERR_BUFF_ERROR;

	return ( 0 );
}

// --
