
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Commands.h"
#include "Radio.h"

// --

void
Cmd_SetOptions( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct TagItem *tags;
	struct TagItem *tag;
	static const U8 zero[6] = { 0 };

	U8 bssid[6];
	U32 channel;
	U32 aid;
	U32 encryption;
	S32 has_aid;
	S32 operational;
	S32 wpa;
	S32 association_changed;
	S32 encryption_set;
	S32 sae;
	S32 reconfigure;

	tags = ioreq->ios2_Data;
	channel = 0;
	aid = unit->unit_AssocID;
	encryption = unit->unit_Encryption;
	has_aid = FALSE;
	operational = -1;
	wpa = -1;
	encryption_set = FALSE;
	sae = FALSE;

	if ( ! tags )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	memcpy( bssid, unit->unit_BSSID, sizeof( bssid ) );

	while(( tag = NextTagItem( & tags ) ))
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
				has_aid = TRUE;
				break;

			case S2INFO_Encryption:
				if ( ( tag->ti_Data != S2ENC_NONE ) && ( tag->ti_Data != S2ENC_CCMP ) &&
					( tag->ti_Data != S2ENC_TKIP ) )
				{
					goto invalid;
				}

				encryption = tag->ti_Data;
				encryption_set = TRUE;
				break;

			case USB2WIRELESSINFO_SAE:
				sae = !! tag->ti_Data;
				break;

			case S2INFO_WPAInfo:
				wpa = !! tag->ti_Data;
				break;

			case S2INFO_OperState:
				operational = !! tag->ti_Data;
				break;

			// The Soft-MAC supplicant supplies these in its management frames.
			case S2INFO_SSID:
			case S2INFO_AuthTypes:
			case S2INFO_BeaconInterval:
				break;

			case S2INFO_DefaultKeyNo:
				if ( tag->ti_Data != 0 )
				{
					goto invalid;
				}

				break;

			case S2INFO_PortType:
				if ( tag->ti_Data != 7 )
				{
					goto invalid;
				}
				break;

			case S2INFO_Band:
				if ( ( tag->ti_Data != 1 ) && ( tag->ti_Data != 2 ) )
				{
					goto invalid;
				}
				break;

			default:
				goto invalid;
		}
	}

	if ( channel == unit->unit_Channel )
	{
		channel = 0;
	}

	association_changed = channel || memcmp( bssid, unit->unit_BSSID, sizeof( bssid ) );

	if ( ( ! has_aid ) && ( association_changed ) )
	{
		aid = 0;
	}

	association_changed |= aid != unit->unit_AssocID;

	if ( wpa == TRUE && encryption == S2ENC_NONE )
	{
		encryption = S2ENC_CCMP;
	}
	else if ( ( wpa == FALSE ) && ( ! encryption_set ) )
	{
		encryption = S2ENC_NONE;
	}

	// A live protected association cannot be changed to a different cipher.
	if ( ( ! association_changed ) && ( aid ) && ( unit->unit_Encryption != S2ENC_NONE ) &&
		( encryption != unit->unit_Encryption ) )
	{
		goto invalid;
	}

	if ( ( sae && encryption != S2ENC_CCMP ) ||
		( aid && sae != unit->unit_SAE ) )
	{
		goto invalid;
	}

	if ( ( sae ) && ( operational == TRUE ) &&
		( ( ! aid ) || ( association_changed ) || ( ! unit->unit_Keys[0].active ) ||
			( unit->unit_Keys[0].type != S2ENC_CCMP ) ||
			( ! ( unit->unit_Keys[1].active || unit->unit_Keys[2].active || unit->unit_Keys[3].active ) ) ||
			( ! ( unit->unit_IGTK[0].active || unit->unit_IGTK[1].active ) ) ) )
	{
		goto invalid;
	}

	if ( ( aid ) && ( ! memcmp( bssid, zero, sizeof( bssid ) ) ) )
	{
		goto invalid;
	}

	if ( ( encryption == S2ENC_TKIP ) && ( ( ! IRTL8188Timer ) ||
		( RTL8188EU_TKIPTime() < unit->unit_TKIPBlockedUntil ) ) )
	{
		goto invalid;
	}

	if ( ! unit->unit_RadioReady )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		return;
	}

	reconfigure = association_changed || ( encryption != unit->unit_Encryption ) || ( sae != unit->unit_SAE );

	if ( reconfigure )
	{
		RTL8188EU_StopRX( unit );

		RTL8188EU_DropLink( unit );

		if ( ( channel && ! RTL8188EU_TuneRadio( unit, channel ) ) ||
			( ! RTL8188EU_SetAssociation( unit, bssid, aid ) ) )
		{
			ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
			ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;
			unit->unit_RadioReady = FALSE;
			unit->unit_TxReady = FALSE;

			Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );

			Sana2_AbortPending( & unit->unit_Pending, NULL, NULL, IOERR_ABORTED, 0 );

			return;
		}

		unit->unit_Encryption = encryption;
		unit->unit_SAE = sae;
	}

	if ( operational >= 0 )
	{
		unit->unit_RequireOperState = TRUE;
		unit->unit_Operational = operational && unit->unit_AssocID;

		if ( ( ! unit->unit_Operational ) && ( unit->unit_Online ) )
		{
			RTL8188EU_SetOnline( unit, FALSE );

			RTL8188EU_AbortData( unit, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE, TRUE );
		}
	}

	if ( ( ! unit->unit_Online ) && ( unit->unit_AssocID ) && ( unit->unit_Configured ) &&
		( unit->unit_TxReady ) && ( RTL8188EU_DeferredLink( unit ) ? unit->unit_Operational : reconfigure ) )
	{
		RTL8188EU_SetOnline( unit, TRUE );
	}

	RTL8188EU_SubmitRX( unit );

	return;

invalid:

	ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
	ioreq->ios2_WireError = S2WERR_INVALID_OPTION;
}

// --
