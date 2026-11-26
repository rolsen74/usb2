
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "Commands.h"
#include "WiFiCommand.h"
#include <usb2wifi.h>

// --

static S32
CompleteRX( struct EthernetUnit *unit, struct USB2_EPResource *rx, struct USB2_IORequest *ioreq )
{
	S32 error;

	error = ioreq->io_Error;

	if ( ( ! error ) && ( ioreq->io_Actual > ioreq->io_Length ) )
	{
		error = IOERR_BADLENGTH;
	}

	if ( error == USB2Err_Host_Timeout )
	{
		return ( TRUE );
	}

	if ( error == USB2Err_Host_Stall )
	{
		Wireless_StopRX( rx, &unit->unit_RxPending );

		if ( unit->unit_USB2->USB2_EPRes_Destall( rx ) )
		{
			DebugPrintF( "RTL8811AU: bulk receive stall cleared\n" );

			return ( TRUE );
		}
	}

	ObtainSemaphore( & unit->unit_CommandLock );

	if ( ! error )
	{
		RTL8811AU_DecodeRX( unit, ioreq->io_Data, ioreq->io_Actual );
	}
	else
	{
		unit->unit_RadioReady = FALSE;
		unit->unit_TxReady = FALSE;

		RTL8811AU_SetOnline( unit, FALSE );

		DebugPrintF( "RTL8811AU: bulk receive stopped, USB error %ld\n", error );

		Sana2_AbortPackets( & unit->unit_Pending, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );

		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX );
	}

	ReleaseSemaphore( & unit->unit_CommandLock );

	return ( ! error );
}

// --

static void
ProcessCommands( struct EthernetUnit *unit, const struct RTL8811AU_Board *board,
	struct USB2_EPResource *rx )
{
	struct IOSana2Req *ioreq;

	S32 start;

	ObtainSemaphore( & unit->unit_CommandLock );

	start = unit->unit_StartAuto && unit->unit_TxReady && ! unit->unit_AssocID;
	unit->unit_StartAuto = FALSE;

	ReleaseSemaphore( & unit->unit_CommandLock );

	if ( start )
	{
		Wireless_StartAuto( USB2WIFI_RTL8811AU, unit->unit_UnitNumber );
	}

	ObtainSemaphore( & unit->unit_CommandLock );

	for( ioreq = (PTR) GetHead( & unit->unit_Pending ); ioreq; ioreq = (PTR) GetSucc( (PTR) ioreq ) )
	{
		switch( ioreq->ios2_Req.io_Command )
		{
			case S2_SETOPTIONS:
				Remove( (PTR) ioreq );

				Cmd_SetOptions( unit, board, ioreq, rx );

				if ( ( rx ) && ( unit->unit_RadioReady ) )
				{
					Wireless_SubmitRX( rx, &unit->unit_RxPending );
				}

				goto reply;

			case S2_SETKEY:
				Remove( (PTR) ioreq );

				Cmd_SetKey( unit, ioreq );

				goto reply;

			case USB2WIRELESS_CMD_PROMPT:
				Remove( (PTR) ioreq );

				ReleaseSemaphore( & unit->unit_CommandLock );

				Wireless_StartPassword( ioreq, USB2WIFI_RTL8811AU, unit->unit_UnitNumber );

				ReplyMsg( (PTR) ioreq );

				return;

			case CMD_WRITE:
			case S2_MULTICAST:
			case S2_BROADCAST:
			case S2_WRITEMGMT:
				Remove( (PTR) ioreq );

				RTL8811AU_Transmit( unit, ioreq );

				goto reply;
		}
	}

	ReleaseSemaphore( & unit->unit_CommandLock );

	return;

reply:

	ReleaseSemaphore( & unit->unit_CommandLock );

	ReplyMsg( (PTR) ioreq );
}

// --

S32
Process_Main( struct EthernetUnit *unit, struct USB2_Register *reg,
	struct USB2_EPResource *rx, const struct RTL8811AU_Board *board )
{
	struct USB2_NotifyMessage *notice;
	struct USB2_Function copy;
	struct USB2_IORequest *ioreq;
	struct USB2IFace *usb2;

	S32 detached;

	detached = FALSE;
	usb2 = unit->unit_USB2;

	if ( rx )
	{
		ObtainSemaphore( & unit->unit_CommandLock );

		unit->unit_RadioReady = TRUE;

		ReleaseSemaphore( & unit->unit_CommandLock );

		Wireless_SubmitRX( rx, &unit->unit_RxPending );

		DebugPrintF( "RTL8811AU: listening for beacons on channel 1\n" );
	}

	while( ! ( SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C ) )
	{
		while( ( notice = (PTR) GetMsg( reg->Stack_MsgPort ) ) )
		{
			detached |= ( notice->nm_Command == USBNC_Detached );

			ReplyMsg( (PTR) notice );
		}

		if ( ( detached ) || ( usb2->USB2_Fkt_FindTags(
			USB2Tag_Find_FunctionID, & reg->FKT_ID,
			USB2Tag_Find_Copy, & copy, TAG_END ) != & copy ) )
		{
			detached = TRUE;

			break;
		}

		if ( rx )
		{
			while( ( ioreq = Wireless_TakeRX( rx, &unit->unit_RxPending ) ) )
			{
				if ( ! CompleteRX( unit, rx, ioreq ) )
				{
					Wireless_StopRX( rx, &unit->unit_RxPending );

					break;
				}
			}

			if ( unit->unit_RadioReady )
			{
				Wireless_SubmitRX( rx, &unit->unit_RxPending );
			}
		}

		ProcessCommands( unit, board, rx );

		Delay( 1 );
	}

	Wireless_StopRX( rx, &unit->unit_RxPending );

	return ( detached );
}

// --
