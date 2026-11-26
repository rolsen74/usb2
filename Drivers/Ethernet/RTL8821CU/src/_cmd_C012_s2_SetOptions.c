
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

// --

static S32
RecoverPipes( struct EthernetUnit *unit, struct USB2IFace *usb2 )
{
	U32 pipe;
	U32 pipe_bit;

	if ( unit->unit_RxStalled )
	{
		if ( ! usb2->USB2_EPRes_Destall( unit->unit_Bulk_In ) )
		{
			return ( FALSE );
		}

		unit->unit_RxStalled = FALSE;
	}

	for( pipe = 0; pipe < RTL8821CU_TX_PIPES; pipe++ )
	{
		pipe_bit = 1U << pipe;

		if ( unit->unit_TxStalled & pipe_bit )
		{
			if ( ! usb2->USB2_EPRes_Destall( unit->unit_Bulk_Out[pipe] ) )
			{
				return ( FALSE );
			}

			unit->unit_TxStalled &= ~pipe_bit;
		}
	}

	return ( TRUE );
}

// --

void
Cmd_SetOptions( struct EthernetUnit *unit, struct USB2IFace *usb2, struct IOSana2Req *ioreq )
{
	static const U8 empty[6] = { 0 };

	struct TagItem *tags;
	struct TagItem *tag;

	U8 bssid[6];
	U32 channel;
	U32 aid;
	U32 encryption;
	S32 operational;
	S32 changed;
	S32 aid_set;
	S32 recover;
	S32 handshake;
	S32 keep_receive;

	tags = ioreq->ios2_Data;
	channel = unit->unit_Channel;
	aid = unit->unit_AssocID;
	encryption = unit->unit_Encryption;
	operational = -1;
	aid_set = FALSE;
	recover = ! unit->unit_RadioReady;

	if ( ! tags )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	memcpy( bssid, unit->unit_BSSID, sizeof( bssid ) );

	while( ( tag = NextTagItem( & tags ) ) )
	{
		switch( tag->ti_Tag )
		{
			case S2INFO_Channel:
				if ( ( tag->ti_Data < 1 ) || ( tag->ti_Data > 11 ) )
				{
					goto invalid;
				}

				channel = tag->ti_Data;

				break;

			case S2INFO_BSSID:
				if ( ! tag->ti_Data )
				{
					goto invalid;
				}

				memcpy( bssid, (PTR) tag->ti_Data, sizeof( bssid ) );

				if ( bssid[0] & 1 )
				{
					goto invalid;
				}

				break;

			case S2INFO_AssocID:
				if ( tag->ti_Data > 2007 )
				{
					goto invalid;
				}

				aid = tag->ti_Data;
				aid_set = TRUE;

				break;

			case S2INFO_Encryption:
				if ( ( tag->ti_Data != S2ENC_NONE ) && ( tag->ti_Data != S2ENC_CCMP ) )
				{
					goto invalid;
				}

				encryption = tag->ti_Data;

				break;

			case S2INFO_OperState:
				operational = tag->ti_Data != 0;

				break;

			case S2INFO_PortType:
				if ( tag->ti_Data != S2PORT_MANAGED )
				{
					goto invalid;
				}

				break;

			case S2INFO_Band:
				if ( tag->ti_Data != 1 )
				{
					goto invalid;
				}

				break;

			case S2INFO_SSID:
			case S2INFO_AuthTypes:
			case S2INFO_BeaconInterval:
				break;

			default:
				goto invalid;
		}
	}

	changed = channel != unit->unit_Channel || memcmp( bssid, unit->unit_BSSID, sizeof( bssid ) );

	if ( ( changed ) && ( ! aid_set ) )
	{
		aid = 0;
	}

	changed |= ( aid != unit->unit_AssocID ) || ( recover );

	if ( ( ( aid ) && ( ! memcmp( bssid, empty, sizeof( bssid ) ) ) ) ||
		( ( ! changed ) && ( aid ) && ( encryption != unit->unit_Encryption ) ) ||
		( ( operational == TRUE ) && ( ( ! aid ) ||
			( ( encryption == S2ENC_CCMP ) && ( ( changed ) || ( ! unit->unit_Keys[0].Active ) ) ) ) ) )
	{
		goto invalid;
	}

	if ( changed )
	{
		handshake = ( encryption == S2ENC_CCMP ) && ( memcmp( bssid, empty, sizeof( bssid ) ) );
		keep_receive = ( ! recover ) && ( handshake ) && ( unit->unit_Handshake ) &&
			( ! unit->unit_AssocID ) && ( aid ) && ( channel == unit->unit_Channel ) &&
			( ! memcmp( bssid, unit->unit_BSSID, sizeof( bssid ) ) );

		if ( ! RTL8821CU_DrainTX( unit ) )
		{
			if ( ! RTL8821CU_AdapterPresent( usb2, unit->unit_USB_Register->FKT_ID ) )
			{
				RTL8821CU_Detach( unit );
			}

			ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
			ioreq->ios2_WireError = unit->unit_Detached ? S2WERR_UNIT_DISCONNECTED : S2WERR_RCVREL_HDW_ERR;

			return;
		}

		if ( ! keep_receive )
		{
			RTL8821CU_StopRX( unit );

			unit->unit_EAPOLLength = 0;
		}

		RTL8821CU_SetOnline( unit, FALSE );

		unit->unit_Operational = FALSE;
		unit->unit_Handshake = FALSE;

		memset( unit->unit_Keys, 0, sizeof( unit->unit_Keys ) );

		if ( ( ( recover ) && ( ! RecoverPipes( unit, usb2 ) ) ) ||
			( ( ( recover ) || ( channel != unit->unit_Channel ) ) && ( ! RTL8821CU_TuneRadio( unit, channel ) ) ) ||
			( ! RTL8821CU_SetAssociation( unit, bssid, aid, ( aid ) || ( handshake ) ) ) ||
			( ! RTL8821CU_SetTxPower( unit ) ) )
		{
			if ( ! RTL8821CU_AdapterPresent( usb2, unit->unit_USB_Register->FKT_ID ) )
			{
				RTL8821CU_Detach( unit );

				ReportUnitOffline( unit, ioreq );

				return;
			}

			unit->unit_RadioReady = FALSE;
			unit->unit_TxReady = FALSE;
			unit->unit_EAPOLLength = 0;
			ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
			ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;

			Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );

			Sana2_AbortPending( & unit->unit_Pending, NULL, NULL, IOERR_ABORTED, 0 );

			return;
		}

		unit->unit_RadioReady = TRUE;
		unit->unit_Handshake = handshake;

		if ( ( handshake ) && ( aid ) )
		{
			DebugPrintF( "RTL8821CU: associated, AID %lu, waiting for WPA2 keys\n", aid );
		}

		if ( recover )
		{
			DebugPrintF( "RTL8821CU: radio recovered on channel %lu\n", channel );
		}
	}

	unit->unit_Encryption = encryption;

	if ( operational >= 0 )
	{
		unit->unit_RequireOperState = TRUE;
		unit->unit_Operational = operational;
	}

	if ( ( changed ) || ( operational == FALSE ) )
	{
		Sana2_AbortPackets( & unit->unit_Pending, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );
	}

	if ( operational == TRUE )
	{
		unit->unit_ManualOffline = FALSE;
		unit->unit_Handshake = FALSE;
		unit->unit_EAPOLLength = 0;
	}

	RTL8821CU_SetOnline( unit, ( ! unit->unit_ManualOffline ) && ( RTL8821CU_CanGoOnline( unit ) ) );

	return;

invalid:

	ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
	ioreq->ios2_WireError = S2WERR_INVALID_OPTION;
}

// --
