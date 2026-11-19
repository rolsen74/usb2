
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"
#include <devices/usb2rtl8188.h>

// --

void
RTL8188EU_Transmit( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	static const U8 snap[6] = { 0xaa, 0xaa, 3, 0, 0, 0 };
	struct USB2_EPResource *resource;
	struct USB2_IORequest *usbio;

	U8 ethernet[RTL8188EU_MTU + 14];
	U8 *buffer;
	U8 frame[RTL8188EU_FRAME_MAX + RTL8188EU_EXT_IV_SIZE + RTL8188EU_TKIP_MIC + RTL8188EU_TKIP_ICV];
	U8 *destination;
	U32 length;
	U32 pad;
	U32 ethernet_size;
	U32 size;
	U32 type;
	U32 i;
	U16 checksum;
	S32 management;
	S32 clear_eapol;

	management = ioreq->ios2_Req.io_Command == S2_WRITEMGMT;
	type = 0;
	clear_eapol = FALSE;

	if ( ( ! unit->unit_TxReady ) || ( ! management && ! unit->unit_Online ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		return;
	}

	resource = unit->unit_Bulk_Tx_Resource;
	buffer = resource->Buffers[0];
	length = management ? ioreq->ios2_DataLength : ioreq->ios2_DataLength + 32;

	if ( ( ! management ) && ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW ) )
	{
		length -= 14;
	}

	if ( ! resource->EndPointMax )
	{
		ioreq->ios2_Req.io_Error = IOERR_UNITBUSY;

		return;
	}

	ethernet_size = 0;

	if ( management )
	{
		memcpy( frame, ioreq->ios2_Data, length );
	}
	else
	{
		destination = ioreq->ios2_DstAddr;
		type = ioreq->ios2_PacketType;

		if ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW )
		{
			if ( ! RTL8188EU_CopyPacket( unit, ioreq, ethernet, ioreq->ios2_Data, ioreq->ios2_DataLength, FALSE ) )
			{
				goto copy_failed;
			}

			destination = ethernet;
			type = ( (U32) ethernet[12] << 8 ) | ethernet[13];

			memcpy( frame + 32, ethernet + 14, ioreq->ios2_DataLength - 14 );
		}
		else if ( ! RTL8188EU_CopyPacket( unit, ioreq, frame + 32, ioreq->ios2_Data, ioreq->ios2_DataLength, FALSE ) )
		{
			goto copy_failed;
		}

		if ( ( ioreq->ios2_Req.io_Command == S2_MULTICAST ) ||
			( ioreq->ios2_Req.io_Command == S2_BROADCAST ) )
		{
			destination = ioreq->ios2_DstAddr;
		}

		memset( frame, 0, 24 );
		frame[0] = 0x08;
		frame[1] = 0x01;

		memcpy( frame + 4, unit->unit_BSSID, 6 );
		memcpy( frame + 10, unit->unit_StationAddress, 6 );
		memcpy( frame + 16, destination, 6 );
		memcpy( frame + 24, snap, sizeof( snap ) );

		frame[30] = type >> 8;
		frame[31] = type;
		ethernet_size = length - 32 + 14;

		// The access point may still be waiting for message 4 before activating its PTK.
		clear_eapol = ( ioreq->ios2_Req.io_Command == CMD_WRITE ) &&
			( ! ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW ) ) &&
			( unit->unit_Encryption == S2ENC_CCMP ) && ( type == RTL8188EU_EAPOL ) &&
			( length == 32 + 99 ) && ( frame[33] == 3 ) && ( frame[34] == 0 ) &&
			( frame[35] == 95 ) && ( frame[36] == 2 ) && ( frame[37] == 3 ) &&
			( frame[38] == 0x0a ) && ( frame[129] == 0 ) && ( frame[130] == 0 );

		if ( ( ! clear_eapol ) && ( ! RTL8188EU_ProtectData( unit, frame, & length, type ) ) )
		{
			ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;

			return;
		}
	}

	pad = ( ( length + RTL8188EU_TX_DESC_SIZE ) % resource->EndPointMax == 0 ) ? 8 : 0;
	size = RTL8188EU_TX_DESC_SIZE + pad + length;

	if ( size > resource->BufferSize )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;

		return;
	}

	memcpy( buffer + RTL8188EU_TX_DESC_SIZE + pad, frame, length );

	memset( buffer, 0, RTL8188EU_TX_DESC_SIZE + pad );
	buffer[0] = length;
	buffer[1] = length >> 8;
	buffer[2] = RTL8188EU_TX_DESC_SIZE;
	buffer[3] = 0x8c | ( frame[4] & 1 );
	buffer[5] = management ? 0x12 : 0;
	buffer[6] = 6;
	buffer[7] = pad ? 0x04 : 0;
	buffer[15] = 0x80;
	buffer[16] = 0x80;
	buffer[17] = 0x01;
	checksum = 0;

	for( i = 0; i < RTL8188EU_TX_DESC_SIZE; i += 2 )
	{
		checksum ^= buffer[i] | ( (U16) buffer[i + 1] << 8 );
	}

	buffer[28] = checksum;
	buffer[29] = checksum >> 8;

	usbio = resource->IORequests[0];
	usbio->io_Command = CMD_WRITE;
	usbio->io_Data = buffer;
	usbio->io_Length = size;
	usbio->io_TimeOut = 500000;

	DoIO( (PTR) usbio );

	if ( ( ! management ) && ( type == RTL8188EU_EAPOL ) )
	{
		USBINFO( "RTL8188EU: EAPOL TX length %lu, flags %02lx, key info %02lx%02lx, clear %lu, USB error %ld, actual %lu",
			(U32) ioreq->ios2_DataLength, (U32) ioreq->ios2_Req.io_Flags,
			(U32) ( ioreq->ios2_DataLength >= 7 ? ( (U8 *) ioreq->ios2_Data )[5] : 0 ),
			(U32) ( ioreq->ios2_DataLength >= 7 ? ( (U8 *) ioreq->ios2_Data )[6] : 0 ),
			(U32) clear_eapol, (S32) usbio->io_Error, usbio->io_Actual );
	}

	if ( ( usbio->io_Error != USB2Err_NoError ) || ( usbio->io_Actual != size ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;

		if ( usbio->io_Error == USB2Err_Host_Stall )
		{
			USB2_EPRes_Destall( resource );
		}

		SignalSana2Event( unit, S2EVENT_ERROR | S2EVENT_TX | S2EVENT_HARDWARE );
	}
	else if ( ! management )
	{
		unit->unit_PacketsTx++;

		RTL8188EU_UpdateThroughput( unit, ethernet_size, 0 );
	}

	return;

copy_failed:

	ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
	ioreq->ios2_WireError = S2WERR_BUFF_ERROR;
}

// --
