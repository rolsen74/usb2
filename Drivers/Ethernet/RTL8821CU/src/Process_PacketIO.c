
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
RadioOffline( struct EthernetUnit *unit, U32 event )
{
	RTL8821CU_StopRX( unit );

	unit->unit_RadioReady = FALSE;
	unit->unit_TxReady = FALSE;
	unit->unit_Operational = FALSE;
	unit->unit_Handshake = FALSE;
	unit->unit_EAPOLLength = 0;

	RTL8821CU_SetOnline( unit, FALSE );

	Sana2_AbortPackets( & unit->unit_Pending, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );

	Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE | event );
}

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
			RTL8821CU_Detach( unit );

			return;
		}
		else if ( error == USB2Err_Host_Stall )
		{
			if ( ( unit->unit_RxStalled ) || ( ! usb2->USB2_EPRes_Destall( resource ) ) )
			{
				unit->unit_RxStalled = TRUE;

				DebugPrintF( "RTL8821CU: bulk receive stalled, radio offline\n" );

				RadioOffline( unit, S2EVENT_RX );

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

	if ( ! unit->unit_RadioReady )
	{
		return;
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
	U32 pipe;
	U32 pipe_bit;
	S32 management;
	S32 error;

	management = sana->ios2_Req.io_Command == S2_WRITEMGMT;

	if ( ( ! unit->unit_TxReady ) || ( ! unit->unit_RadioReady ) ||
		( ( ! management ) && ( ! RTL8821CU_DataAllowed( unit, sana ) ) ) )
	{
		ReportUnitOffline( unit, sana );

		return;
	}

	pipe = management ? RTL8821CU_TX_MANAGEMENT : RTL8821CU_TX_BEST_EFFORT;
	pipe_bit = 1U << pipe;
	resource = unit->unit_Bulk_Out[pipe];
	ioreq = resource->IORequests[0];
	buffer = resource->Buffers[0];

	length = RTL8821CU_BuildTX( unit, sana, buffer, & type, & payload );

	if ( ! length )
	{
		if ( sana->ios2_WireError == S2WERR_BUFF_ERROR )
		{
			Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_TX | S2EVENT_BUFF );
		}

		return;
	}

	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Data = buffer;
	ioreq->io_Length = length;
	ioreq->io_TimeOut = 500000;
	// Management and best-effort traffic use the high and low page queues.
	unit->unit_TxQueued |= management ? 1U : 2U;

	error = DoIO( (PTR) ioreq );

	if ( ( management ) && ( buffer[RTL8821CU_TX_DESC_SIZE] == 0xb0 ) )
	{
		DebugPrintF( "RTL8821CU: authentication TX USB %ld, %lu/%lu bytes on pipe %lu\n",
			error, ioreq->io_Actual, length, pipe );
	}

	if ( ( ! management ) && ( type == RTL8821CU_EAPOL ) && ( unit->unit_Handshake ) )
	{
		DebugPrintF( "RTL8821CU: EAPOL TX %lu bytes, USB error %ld\n", payload, error );
	}

	if ( error == USB2Err_Device_Detached )
	{
		RTL8821CU_Detach( unit );

		ReportUnitOffline( unit, sana );

		return;
	}

	if ( ( error ) || ( ioreq->io_Actual != length ) )
	{
		sana->ios2_Req.io_Error = S2ERR_TX_FAILURE;

		if ( error == USB2Err_Host_Stall )
		{
			if ( ( unit->unit_TxStalled & pipe_bit ) || ( ! usb2->USB2_EPRes_Destall( resource ) ) )
			{
				unit->unit_TxStalled |= pipe_bit;

				DebugPrintF( "RTL8821CU: bulk transmit pipe %lu stalled, radio offline\n", pipe );

				RadioOffline( unit, S2EVENT_TX );

				return;
			}

			unit->unit_TxStalled |= pipe_bit;
		}

		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_TX | S2EVENT_HARDWARE );

		return;
	}

	unit->unit_TxStalled &= ~pipe_bit;

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
		RTL8821CU_DeliverEAPOL( unit );

		PollReceive( unit, usb2 );
	}

	if ( unit->unit_Detached )
	{
		return;
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
