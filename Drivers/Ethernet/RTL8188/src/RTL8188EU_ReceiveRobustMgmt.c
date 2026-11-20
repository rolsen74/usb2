
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

#define IEEE80211_ACTION 0xd0
#define IEEE80211_SA_QUERY 8

// --

static void
ReplySAQuery( struct EthernetUnit *unit, const U8 *body )
{
	struct IOSana2Req request;

	U8 response[28 + WIRELESS_CCMP_IV + WIRELESS_CCMP_MIC];
	U32 length;

	memset( response, 0, 24 );

	response[0] = IEEE80211_ACTION;
	response[24] = IEEE80211_SA_QUERY;
	response[25] = 1;
	response[26] = body[2];
	response[27] = body[3];
	length = 28;

	memcpy( response + 4, unit->unit_BSSID, 6 );

	memcpy( response + 10, unit->unit_StationAddress, 6 );

	memcpy( response + 16, unit->unit_BSSID, 6 );

	if ( ( ! unit->unit_Keys[0].active ) ||
		( ! RTL8188EU_ProtectData( unit, response, & length, 0 ) ) )
	{
		return;
	}

	memset( & request, 0, sizeof( request ) );

	request.ios2_Req.io_Command = S2_WRITEMGMT;
	request.ios2_Data = response;
	request.ios2_DataLength = length;

	RTL8188EU_Transmit( unit, & request );
}

// --

void
RTL8188EU_ReceiveRobustMgmt( struct EthernetUnit *unit, U8 *frame, U32 length )
{
	static const U8 broadcast[6] = { 255, 255, 255, 255, 255, 255 };

	const U8 *body;
	U32 header;
	S32 group;

	if ( ( ! unit->unit_AssocID ) || ( length < 26 ) || ( frame[1] & 0x87 ) ||
		( frame[22] & 0x0f ) || ( memcmp( frame + 10, unit->unit_BSSID, 6 ) ) ||
		( memcmp( frame + 16, unit->unit_BSSID, 6 ) ) )
	{
		return;
	}

	group = ! memcmp( frame + 4, broadcast, sizeof( broadcast ) );

	if ( ( ! group ) && ( memcmp( frame + 4, unit->unit_StationAddress, 6 ) ) )
	{
		return;
	}

	header = 24;

	if ( unit->unit_SAE )
	{
		if ( group )
		{
			if ( ( frame[1] & 0x40 ) || ( ! Wireless_CheckBIP( unit->unit_IGTK, frame, length ) ) )
			{
				return;
			}

			length -= WIRELESS_MMIE_LENGTH;
		}
		else if ( ( ! ( frame[1] & 0x40 ) ) ||
			( ! RTL8188EU_UnprotectData( unit, frame, & length, & header ) ) )
		{
			return;
		}
	}
	else if ( ( group ) || ( frame[1] & 0x40 ) )
	{
		return;
	}

	body = frame + header;

	if ( frame[0] == IEEE80211_ACTION )
	{
		if ( ( ! group ) && ( unit->unit_SAE ) && ( unit->unit_Operational ) &&
			( length >= header + 4 ) && ( body[0] == IEEE80211_SA_QUERY ) && ( body[1] == 0 ) )
		{
			ReplySAQuery( unit, body );
		}

		return;
	}

	USBINFO( "RTL8188EU: AP %s, reason %lu", frame[0] == 0xa0 ? "disassociated" : "deauthenticated",
		(U32) body[0] | ( (U32) body[1] << 8 ) );

	RTL8188EU_DropLink( unit );
}

// --
