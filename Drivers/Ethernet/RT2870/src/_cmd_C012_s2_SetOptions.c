
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

void
Cmd_SetOptions( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct TagItem *tags;
	struct TagItem *tag;

	static const U8 empty[6] = { 0 };
	const U8 *bssid;
	U32 aid;
	U32 encryption;
	S32 operational;
	S32 sae;
	S32 tkip;
	U32 channel;
	S32 valid;
	S32 error;

	tags = ioreq->ios2_Data;
	bssid = empty;
	aid = 0;
	encryption = 0;
	operational = FALSE;
	sae = FALSE;
	channel = 0;
	valid = TRUE;

	if ( ! tags )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	while( ( tag = NextTagItem( & tags ) ) && ( valid ) )
	{
		switch ( tag->ti_Tag )
		{
			case S2INFO_Channel:
				channel = tag->ti_Data;
				valid = channel <= RT2870_MAX_CHANNEL;
				break;

			case S2INFO_BSSID:
				bssid = (PTR) tag->ti_Data;
				valid = bssid && ! ( bssid[0] & 1 );
				break;

			case S2INFO_PortType:
				valid = tag->ti_Data == S2PORT_MANAGED;
				break;

			case S2INFO_AssocID:
				aid = tag->ti_Data;
				valid = aid <= 2007;
				break;

			case S2INFO_Encryption:
				encryption = tag->ti_Data;
				valid = ( ! encryption ) || ( encryption == S2ENC_CCMP ) || ( encryption == S2ENC_TKIP );
				break;

			case USB2RT2870INFO_SAE:
				sae = tag->ti_Data != 0;
				break;

			case S2INFO_OperState:
				operational = tag->ti_Data != 0;
				break;

			default:
				valid = FALSE;
				break;
		}
	}

	tkip = encryption == S2ENC_TKIP;

	if ( ( tkip ) && ( ( sae ) || ( ! IRT2870Timer ) || ( RT2870_TKIPTime() < unit->unit_TKIPBlockedUntil ) ) )
	{
		valid = FALSE;
	}

	if ( ( operational ) && ( ! channel ) )
	{
		channel = unit->unit_Channel;
	}

	if ( ( valid ) && ( encryption ) )
	{
		valid = ( channel ) && ( memcmp( bssid, empty, sizeof( empty ) ) ) &&
			( ( ! aid ) || ( ( channel == unit->unit_Channel ) &&
				( ! memcmp( bssid, unit->unit_Peer, sizeof( empty ) ) ) ) ) &&
			( ( ! aid ) || ( sae == unit->unit_SAE && tkip == unit->unit_TKIP ) ) &&
			( ( ! unit->unit_ActiveKeys ) || ( aid == unit->unit_AssocID ) ) &&
			( ( ! operational ) || ( ! sae ) || ( unit->unit_IGTK[0].active ) || ( unit->unit_IGTK[1].active ) ) &&
			( ( ! operational ) || ( ( aid ) && ( unit->unit_M4Sent ) && ( unit->unit_Configured ) &&
				( unit->unit_ActiveKeys & RT2870_PAIRWISE_KEY ) && ( unit->unit_ActiveKeys & RT2870_GROUP_KEYS ) ) );
	}
	else if ( valid )
	{
		valid = ( ! sae ) && ( ! operational ) && ( ! aid ) && ( ! memcmp( bssid, empty, sizeof( empty ) ) );
	}

	if ( ! valid )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_INVALID_OPTION;

		return;
	}

	if ( channel == unit->unit_Channel )
	{
		if ( ( ! encryption ) && ( unit->unit_AssocID ) )
		{
			RT2870_StopBeacon( unit, FALSE );
		}

		if ( ( encryption ) && ( ! aid ) )
		{
			unit->unit_PeerBeaconInterval = 0;

			if ( ! memcmp( bssid, unit->unit_LastBeaconBSSID, sizeof( empty ) ) )
			{
				unit->unit_PeerBeaconInterval = unit->unit_LastBeaconInterval;
			}
		}

		unit->unit_AssocID = aid;
		unit->unit_SAE = sae;
		unit->unit_TKIP = tkip;

		if ( operational != unit->unit_Operational )
		{
			error = 0;

			if ( ( operational ) && ( ! unit->unit_PeerConfigured ) )
			{
				error = RT2870_SetStationPeer( unit, TRUE );
			}

			if ( ! error )
			{
				error = RT2870_SetStationTiming( unit, operational ?
					( unit->unit_PeerBeaconInterval ? unit->unit_PeerBeaconInterval : 100 ) : 0 );
			}

			if ( error )
			{
				USBERROR( "RT2870: station link setup failed, USB error %ld", error );

				RT2870_DropLink( unit, error == USB2Err_Device_Detached );

				ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
				ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;

				return;
			}

			unit->unit_Operational = operational;

			RT2870_SetOnline( unit, operational );

			if ( operational )
			{
				USBINFO( "RT2870: %s data link online on channel %lu, data rate %lu Mbps",
					unit->unit_TKIP ? "WPA" : ( unit->unit_SAE ? "WPA3" : "WPA2" ), channel, unit->unit_DataOFDM ? 6UL : 1UL );
			}
		}

		return;
	}

	RT2870_StopBeacon( unit, FALSE );

	if ( ( channel ) && ( ( ! unit->unit_RxCheckReady ) ||
		( ! RT2870_TuneChannel( unit->unit_USB_Register->Res_Control, channel ) ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;
		channel = 0;
	}

	ObtainSemaphore( & unit->unit_CommandLock );

	unit->unit_Channel = channel;
	unit->unit_SAE = channel && sae;
	unit->unit_TKIP = channel && tkip;

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --
