
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define IEEE80211_ASSOC_REQUEST	0x00
#define IEEE80211_PROBE_REQUEST		0x40
#define IEEE80211_AUTH_REQUEST		0xb0
#define IEEE80211_MGMT_MIN		30
#define IEEE80211_DEST			4
#define IEEE80211_SOURCE		10
#define IEEE80211_BSSID			16

// --

static S32
SupportsOFDM( const U8 *frame, U32 length )
{
	U32 offset;
	U32 size;
	U32 index;

	offset = 28; // Association request information elements

	while( length - offset >= 2 )
	{
		size = frame[offset + 1];

		if ( size > length - offset - 2 )
		{
			break;
		}

		if ( ( frame[offset] == 1 ) || ( frame[offset] == 50 ) )
		{
			for( index = 0; index < size; index++ )
			{
				if ( ( frame[offset + 2 + index] & 0x7f ) == 12 )
				{
					return ( TRUE );
				}
			}
		}

		offset += size + 2;
	}

	return ( FALSE );
}

// --

void
RT2870_WriteManagement( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	static const U8 empty[6] = { 0 };
	static const U8 broadcast[6] = { 255, 255, 255, 255, 255, 255 };
	const U8 *frame;
	U32 frameLength;
	S32 error;
	S32 probe;

	frame = ioreq->ios2_Data;
	frameLength = ioreq->ios2_DataLength;

	if ( ( ! unit->unit_Channel ) || ( unit->unit_AssocID ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		return;
	}

	if ( ! frame )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	if ( ( frameLength < IEEE80211_MGMT_MIN ) || ( frameLength > RT2870_MAX_FRAME ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;

		return;
	}

	probe = frame[0] == IEEE80211_PROBE_REQUEST;

	if ( ( ! probe && frame[0] != IEEE80211_AUTH_REQUEST && frame[0] != IEEE80211_ASSOC_REQUEST ) ||
		( frame[1] & 0x43 ) ||
		( probe ? memcmp( frame + IEEE80211_DEST, broadcast, sizeof( broadcast ) ) :
			( ( frame[IEEE80211_DEST] & 1 ) || ! memcmp( frame + IEEE80211_DEST, empty, sizeof( empty ) ) ) ) ||
		( memcmp( frame + IEEE80211_DEST, frame + IEEE80211_BSSID, 6 ) ) ||
		( memcmp( frame + IEEE80211_SOURCE, unit->unit_StationAddress, 6 ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;

		return;
	}

	if ( probe )
	{
		error = RT2870_TransmitFrame( unit, frame, frameLength, broadcast, FALSE, NULL );

		if ( error )
		{
			ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;
			ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;
		}

		return;
	}

	USBINFO( "RT2870: sending management subtype $%02lx to %02lx:%02lx:%02lx:%02lx:%02lx:%02lx",
		(U32) frame[0], (U32) frame[4], (U32) frame[5], (U32) frame[6],
		(U32) frame[7], (U32) frame[8], (U32) frame[9] );

	error = RT2870_TransmitFrame( unit, frame, frameLength, frame + IEEE80211_BSSID, FALSE, "management" );

	if ( ! error )
	{
		memcpy( unit->unit_Peer, frame + IEEE80211_BSSID, sizeof( unit->unit_Peer ) );

		unit->unit_EAPOLLength = 0;

		if ( frame[0] == IEEE80211_ASSOC_REQUEST )
		{
			unit->unit_DataOFDM = SupportsOFDM( frame, frameLength );
		}
	}
	else
	{
		ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;
		ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;
	}
}

// --
