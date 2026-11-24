
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "WiFiCommand.h"
#include <usb2wifi.h>

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
			unit->unit_RxStalled = FALSE;

			RTL8821CU_DecodeRX( unit, resource->Buffers[0], ioreq->io_Actual );
		}
		else if ( error == USB2Err_Device_Detached )
		{
			return;
		}
		else if ( error == USB2Err_Host_Stall )
		{
			if ( ( unit->unit_RxStalled ) || ( ! usb2->USB2_EPRes_Destall( resource ) ) )
			{
				unit->unit_RxStalled = TRUE;
				unit->unit_RadioReady = FALSE;
				unit->unit_TxReady = FALSE;
				unit->unit_Operational = FALSE;

				RTL8821CU_SetOnline( unit, FALSE );

				DebugPrintF( "RTL8821CU: bulk receive stalled, radio offline\n" );

				Sana2_AbortPackets( & unit->unit_Pending, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );

				Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_HARDWARE );

				return;
			}

			unit->unit_RxStalled = TRUE;

			DebugPrintF( "RTL8821CU: bulk receive stall cleared\n" );
		}
		else if ( error != USB2Err_Host_Timeout )
		{
			Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_HARDWARE );
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
	U32 payload;
	U32 type;
	S32 management;
	S32 error;

	management = sana->ios2_Req.io_Command == S2_WRITEMGMT;

	if ( ( ! unit->unit_TxReady ) || ( ! unit->unit_RadioReady ) ||
		( ( ! management ) && ( ! RTL8821CU_DataAllowed( unit, sana ) ) ) )
	{
		ReportUnitOffline( unit, sana );

		return;
	}

	resource = unit->unit_Bulk_Out[management ? RTL8821CU_TX_MANAGEMENT : RTL8821CU_TX_BEST_EFFORT];
	ioreq = resource->IORequests[0];
	buffer = resource->Buffers[0];

	length = RTL8821CU_BuildTX( unit, sana, buffer, & type, & payload );

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

	if ( management )
	{
		return;
	}

	unit->unit_PacketsTx++;

	opener = sana->ios2_BufferManagement;

	Sana2_AccountType( & opener->bm_TrackedTypes, type, payload, FALSE, FALSE );

	Sana2_UpdateThroughput( & unit->unit_Pending, payload + RTL8821CU_ETH_HEADER, 0 );
}

// --

void
Process_PacketIO( struct EthernetUnit *unit, struct USB2IFace *usb2 )
{
	struct IOSana2Req *sana;

	if ( unit->unit_RadioReady )
	{
		PollReceive( unit, usb2 );
	}

	for( sana = (PTR) GetHead( & unit->unit_Pending ); sana; sana = (PTR) GetSucc( (PTR) sana ) )
	{
		switch( sana->ios2_Req.io_Command )
		{
			case S2_SETOPTIONS:
				Remove( (PTR) sana );

				Cmd_SetOptions( unit, usb2, sana );

				if ( ( unit->unit_RadioReady ) && ( ! unit->unit_RxPending ) )
				{
					PollReceive( unit, usb2 );
				}

				ReplyMsg( (PTR) sana );

				return;

			case S2_SETKEY:
				Remove( (PTR) sana );

				Cmd_SetKey( unit, sana );

				ReplyMsg( (PTR) sana );

				return;

			case USB2WIRELESS_CMD_PROMPT:
				Remove( (PTR) sana );

				ReleaseSemaphore( & unit->unit_CommandLock );

				Wireless_StartPassword( sana, USB2WIFI_RTL8821CU, unit->unit_UnitNr );

				ReplyMsg( (PTR) sana );

				ObtainSemaphore( & unit->unit_CommandLock );

				return;

			case S2_WRITEMGMT:
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
