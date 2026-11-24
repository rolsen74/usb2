
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

int
SendPacket( struct devUnit *unit, struct IONode *node )
{
	struct IOSana2Req *request;
	struct USB2_IORequest *usbio;

	U8 *frame;
	U32 length;
	U32 header;
	U32 type;
	U32 options;

	request = node->Sana2IOReq;
	header = ( request->ios2_Req.io_Flags & SANA2IOF_RAW ) ? 0 : 14;

	if ( ( header && request->ios2_DataLength > ETHERNET_PAYLOAD_MTU ) ||
		( ! header && ( request->ios2_DataLength < 14 || request->ios2_DataLength > ETHERNET_MTU ) ) )
	{
		request->ios2_Req.io_Error = S2ERR_MTU_EXCEEDED;
		request->ios2_WireError = S2WERR_GENERIC_ERROR;

		return ( FALSE );
	}

	frame = node->Buffer + RTL8152_TX_DESCRIPTOR_SIZE;
	length = request->ios2_DataLength + header;

	if ( ! Copy_From_Buffer( unit, request, frame + header ) )
	{
		request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		request->ios2_WireError = S2WERR_BUFF_ERROR;

		SignalSanaEvent( unit, S2EVENT_ERROR | S2EVENT_TX | S2EVENT_BUFF );

		return ( FALSE );
	}

	if ( header )
	{
		memcpy( frame, request->ios2_DstAddr, 6 );

		memcpy( frame + 6, unit->unit_CurrentAddress, 6 );

		type = request->ios2_PacketType;
		frame[12] = type >> 8;
		frame[13] = type;
	}

	options = length | RTL8152_TX_SEGMENTS;
	node->Buffer[0] = options;
	node->Buffer[1] = options >> 8;
	node->Buffer[2] = options >> 16;
	node->Buffer[3] = options >> 24;

	memset( node->Buffer + 4, 0, 4 );

	request->ios2_WireError = 0;
	usbio = node->USBRequest;
	usbio->io_Command = CMD_WRITE;
	usbio->io_Data = node->Buffer;
	usbio->io_Length = length + RTL8152_TX_DESCRIPTOR_SIZE;
	node->Active = TRUE;

	IExec->SendIO( (PTR) usbio );

	return ( TRUE );
}

// --
