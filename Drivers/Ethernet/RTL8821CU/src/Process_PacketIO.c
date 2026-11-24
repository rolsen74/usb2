
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
PollReceive( struct EthernetUnit *unit, struct USB2IFace *usb2 )
{
	struct USB2_EPResource *resource;
	struct USB2_IORequest *ioreq;

	S32 error;

	resource = unit->unit_Bulk_In;
	ioreq = resource->IORequests[0];

	if ( unit->unit_RxPending )
	{
		if ( ! CheckIO( (PTR) ioreq ) )
		{
			return;
		}

		error = WaitIO( (PTR) ioreq );

		unit->unit_RxPending = FALSE;

		if ( ( ! error ) && ( ioreq->io_Actual <= resource->BufferSize ) )
		{
			RTL8821CU_DecodeRX( unit, resource->Buffers[0], ioreq->io_Actual );
		}
		else if ( error != USB2Err_Device_Detached )
		{
			if ( error == USB2Err_Host_Stall )
			{
				usb2->USB2_EPRes_Destall( resource );
			}

			Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_HARDWARE );
		}
		else
		{
			return;
		}
	}

	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = resource->Buffers[0];
	ioreq->io_Length = resource->BufferSize;
	ioreq->io_TimeOut = 0;
	unit->unit_RxPending = TRUE;

	SendIO( (PTR) ioreq );
}

// --

static void
Transmit( struct EthernetUnit *unit, struct USB2IFace *usb2, struct IOSana2Req *sana )
{
	struct USB2_EPResource *resource;
	struct USB2_IORequest *ioreq;
	struct BufferManagement *opener;

	U8 *buffer;
	U32 length;
	U32 frame_offset;
	U16 type;
	S32 error;

	resource = unit->unit_Bulk_Out[RTL8821CU_TX_BEST_EFFORT];
	ioreq = resource->IORequests[0];
	buffer = resource->Buffers[0];

	length = RTL8821CU_BuildTX( unit, sana, buffer );

	if ( ! length )
	{
		return;
	}

	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Data = buffer;
	ioreq->io_Length = length;
	ioreq->io_TimeOut = 500000;

	error = DoIO( (PTR) ioreq );

	if ( error == USB2Err_Device_Detached )
	{
		sana->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		sana->ios2_WireError = S2WERR_UNIT_DISCONNECTED;

		return;
	}

	if ( ( error ) || ( ioreq->io_Actual != length ) )
	{
		sana->ios2_Req.io_Error = S2ERR_TX_FAILURE;

		if ( error == USB2Err_Host_Stall )
		{
			usb2->USB2_EPRes_Destall( resource );
		}

		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_TX | S2EVENT_HARDWARE );

		return;
	}

	unit->unit_PacketsTx++;
	frame_offset = RTL8821CU_TX_DESC_SIZE + RTL8821CU_WLAN_HEADER + RTL8821CU_LLC_HEADER;
	type = ( (U16) buffer[frame_offset - 2] << 8 ) | buffer[frame_offset - 1];

	opener = sana->ios2_BufferManagement;

	Sana2_AccountType( & opener->bm_TrackedTypes, type, length - frame_offset, FALSE, FALSE );

	Sana2_UpdateThroughput( & unit->unit_Pending, length - frame_offset + RTL8821CU_ETH_HEADER, 0 );
}

// --

void
Process_PacketIO( struct EthernetUnit *unit, struct USB2IFace *usb2 )
{
	struct IOSana2Req *sana;

	if ( ! unit->unit_Online )
	{
		RTL8821CU_StopRX( unit );

		return;
	}

	PollReceive( unit, usb2 );

	for( sana = (PTR) GetHead( & unit->unit_Pending ); sana; sana = (PTR) GetSucc( (PTR) sana ) )
	{
		switch( sana->ios2_Req.io_Command )
		{
			case CMD_WRITE:
			case S2_MULTICAST:
			case S2_BROADCAST:
				Remove( (PTR) sana );

				Transmit( unit, usb2, sana );

				ReplyMsg( (PTR) sana );

				return;
		}
	}
}

// --
