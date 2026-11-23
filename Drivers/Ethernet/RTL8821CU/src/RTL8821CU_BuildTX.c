
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define TX_LAST_SEGMENT 0x04
#define TX_USE_RATE 0x01
#define TX_RATE_OFDM_6MBPS 4
#define TX_HW_SEQUENCE 0x80

// --

U32
RTL8821CU_BuildTX( struct EthernetUnit *unit, struct IOSana2Req *ioreq, U8 *buffer )
{
	static const U8 empty[RTL8821CU_TX_DESC_SIZE + RTL8821CU_WLAN_HEADER] = { 0 };
	static const U8 snap[] = { 0xaa, 0xaa, 0x03, 0, 0, 0 };

	const U8 *destination;
	U8 ethernet[RTL8821CU_MTU + RTL8821CU_ETH_HEADER];
	U8 *frame;
	U8 *payload;
	U32 length;
	U32 type;

	frame = buffer + RTL8821CU_TX_DESC_SIZE;
	payload = frame + RTL8821CU_WLAN_HEADER + RTL8821CU_LLC_HEADER;
	length = ioreq->ios2_DataLength;
	type = ioreq->ios2_PacketType;
	destination = ioreq->ios2_DstAddr;

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
	length += RTL8821CU_WLAN_HEADER + RTL8821CU_LLC_HEADER;

	// The initial data path uses best-effort and the legacy OFDM basic rate.
	buffer[0] = length;
	buffer[1] = length >> 8;
	buffer[2] = RTL8821CU_TX_DESC_SIZE;
	buffer[3] = TX_LAST_SEGMENT;
	buffer[13] = TX_USE_RATE;
	buffer[16] = TX_RATE_OFDM_6MBPS;
	buffer[33] = TX_HW_SEQUENCE;

	RTL8821CU_ChecksumTX( buffer );

	return ( RTL8821CU_TX_DESC_SIZE + length );

copy_failed:

	ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
	ioreq->ios2_WireError = S2WERR_BUFF_ERROR;

	return ( 0 );
}

// --
