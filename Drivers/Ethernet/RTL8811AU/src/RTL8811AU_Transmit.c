
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

static void
FillDescriptor( U8 *desc, U32 length, U16 sequence, S32 mgmt, S32 multicast, U32 padding )
{
	U32 index;
	U16 checksum;

	memset( desc, 0, RTL8811AU_TX_DESC_SIZE + padding );

	desc[0] = length;
	desc[1] = length >> 8;
	desc[2] = RTL8811AU_TX_DESC_SIZE;
	desc[3] = 0x8c | ( multicast != 0 );
	desc[5] = mgmt ? 0x12 : 0;
	desc[6] = 6;
	desc[7] = padding ? 1 : 0;
	desc[13] = 0x05;
	desc[18] = 0x32;
	desc[37] = sequence << 4;
	desc[38] = sequence >> 4;
	checksum = 0;

	for( index = 0; index < 32; index += 2 )
	{
		checksum ^= desc[index] | ( (U16) desc[index + 1] << 8 );
	}

	desc[28] = checksum;
	desc[29] = checksum >> 8;
}

// --

static S32
MakeDataFrame( struct EthernetUnit *unit, struct IOSana2Req *ioreq, U8 *frame, U32 *length, U32 *type )
{
	static const U8 snap[] = { 0xaa, 0xaa, 3, 0, 0, 0 };

	const U8 *dest;
	U8 ethernet[RTL8811AU_MTU + RTL8811AU_ETH_HEADER];
	U32 payload;

	if ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW )
	{
		if ( ! RTL8811AU_CopyPacket( unit, ioreq, ethernet, ioreq->ios2_Data, ioreq->ios2_DataLength, FALSE ) )
		{
			return ( FALSE );
		}

		dest = ethernet;
		*type = ( (U32) ethernet[12] << 8 ) | ethernet[13];
		payload = ioreq->ios2_DataLength - RTL8811AU_ETH_HEADER;

		memcpy( frame + 32, ethernet + RTL8811AU_ETH_HEADER, payload );
	}
	else
	{
		dest = ioreq->ios2_DstAddr;
		*type = ioreq->ios2_PacketType;
		payload = ioreq->ios2_DataLength;

		if ( ! RTL8811AU_CopyPacket( unit, ioreq, frame + 32, ioreq->ios2_Data, payload, FALSE ) )
		{
			return ( FALSE );
		}
	}

	if ( ( ioreq->ios2_Req.io_Command == S2_MULTICAST ) || ( ioreq->ios2_Req.io_Command == S2_BROADCAST ) )
	{
		dest = ioreq->ios2_DstAddr;
	}

	memset( frame, 0, 24 );

	frame[0] = 0x08;
	frame[1] = 0x01;

	memcpy( frame + 4, unit->unit_BSSID, 6 );
	memcpy( frame + 10, unit->unit_StationAddress, 6 );
	memcpy( frame + 16, dest, 6 );
	memcpy( frame + 24, snap, sizeof( snap ) );

	frame[30] = *type >> 8;
	frame[31] = *type;
	*length = payload + 32;

	return ( TRUE );
}

// --

void
RTL8811AU_Transmit( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct USB2_EPResource *tx;
	struct USB2_IORequest *usbio;

	U8 frame[RTL8811AU_FRAME_MAX + WIRELESS_CCMP_IV + WIRELESS_CCMP_MIC];
	U8 *buffer;
	U32 length;
	U32 padding;
	U32 type;
	U32 payload;
	U16 sequence;
	S32 mgmt;

	tx = unit->unit_Tx;
	mgmt = ioreq->ios2_Req.io_Command == S2_WRITEMGMT;
	type = 0;
	payload = 0;

	if ( ( ! unit->unit_TxReady ) || ( ! unit->unit_RadioReady ) ||
		( ( ! mgmt ) && ( ! RTL8811AU_DataAllowed( unit, ioreq ) ) ) )
	{
		ReportUnitOffline( unit, ioreq );

		return;
	}

	if ( mgmt )
	{
		length = ioreq->ios2_DataLength;

		memcpy( frame, ioreq->ios2_Data, length );
	}
	else
	{
		if ( ! MakeDataFrame( unit, ioreq, frame, & length, & type ) )
		{
			ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
			ioreq->ios2_WireError = S2WERR_BUFF_ERROR;

			return;
		}

		payload = length - 32;
	}

	sequence = unit->unit_TxSequence++ & 0x0fff;
	frame[22] = sequence << 4;
	frame[23] = sequence >> 4;

	if ( ( ! mgmt ) && ( ! RTL8811AU_ProtectData( unit, frame, & length, type ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;

		return;
	}

	padding = ( ( length + RTL8811AU_TX_DESC_SIZE ) % tx->EndPointMax ) ? 0 : 8;
	buffer = tx->Buffers[0];

	FillDescriptor( buffer, length, sequence, mgmt, frame[4] & 1, padding );

	memcpy( buffer + RTL8811AU_TX_DESC_SIZE + padding, frame, length );

	usbio = tx->IORequests[0];
	usbio->io_Command = CMD_WRITE;
	usbio->io_Data = buffer;
	usbio->io_Length = RTL8811AU_TX_DESC_SIZE + padding + length;
	usbio->io_TimeOut = 500000;

	DoIO( (PTR) usbio );

	if ( ( usbio->io_Error ) || ( usbio->io_Actual != usbio->io_Length ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;
		ioreq->ios2_WireError = S2WERR_GENERIC_ERROR;

		if ( usbio->io_Error == USB2Err_Host_Stall )
		{
			unit->unit_USB2->USB2_EPRes_Destall( tx );
		}

		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX );
	}
	else if ( ! mgmt )
	{
		unit->unit_PacketsTx++;

		RTL8811AU_AccountType( unit, type, payload, FALSE, FALSE );

		Sana2_UpdateThroughput( & unit->unit_Pending, payload + RTL8811AU_ETH_HEADER, 0 );
	}
}

// --
