
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define IEEE80211_DISASSOC 0xa0
#define IEEE80211_DEAUTH 0xc0
#define IEEE80211_ACTION 0xd0
#define IEEE80211_SA_QUERY 8

// --

static S32
DecryptUnicast( struct EthernetUnit *unit, U8 *frame, U32 length )
{
	const U8 *iv;
	U8 *body;
	U32 i;
	U32 size;
	U64 number;

	iv = frame + 24;

	if ( ( ! ( unit->unit_ActiveKeys & RT2870_PAIRWISE_KEY ) ) || ( ! ( frame[1] & 0x40 ) ) ||
		( length < 26 + WIRELESS_CCMP_IV + WIRELESS_CCMP_MIC ) ||
		( iv[2] ) || ( iv[3] != 0x20 ) )
	{
		return ( FALSE );
	}

	number = iv[0] | ( (U64) iv[1] << 8 );

	for( i = 2; i < 6; i++ )
	{
		number |= (U64) iv[i + 2] << ( 8 * i );
	}

	body = frame + 24 + WIRELESS_CCMP_IV;
	size = length - 24 - WIRELESS_CCMP_IV - WIRELESS_CCMP_MIC;

	if ( ( number <= unit->unit_ManagementRxPN ) ||
		( ! Wireless_CCMP( unit->unit_Keys[0].round_keys, frame, WIRELESS_MGMT, number,
			body, size, body + size, FALSE ) ) )
	{
		return ( FALSE );
	}

	unit->unit_ManagementRxPN = number;

	return ( TRUE );
}

// --

S32
RT2870_ReceiveRobustMgmt( struct EthernetUnit *unit, U8 *frame, U32 length, S32 decrypted )
{
	static const U8 broadcast[] = { 255, 255, 255, 255, 255, 255 };
	U8 response[28 + WIRELESS_CCMP_IV + WIRELESS_TKIP_MIC + WIRELESS_TKIP_ICV];
	const U8 *body;
	U32 responseLength;
	S32 group;

	if ( ( ! unit->unit_AssocID ) || ( length < 26 ) || ( frame[1] & 0x87 ) ||
		( frame[22] & 0x0f ) || ( memcmp( frame + 10, unit->unit_Peer, 6 ) ) ||
		( memcmp( frame + 16, unit->unit_Peer, 6 ) ) )
	{
		return ( FALSE );
	}

	group = ! memcmp( frame + 4, broadcast, 6 );

	if ( ( ! group ) && ( memcmp( frame + 4, unit->unit_StationAddress, 6 ) ) )
	{
		return ( FALSE );
	}

	body = frame + 24;

	if ( unit->unit_SAE )
	{
		if ( ( group ) ? ( ! Wireless_CheckBIP( unit->unit_IGTK, frame, length ) ) :
			( ( decrypted ) || ( ! DecryptUnicast( unit, frame, length ) ) ) )
		{
			return ( FALSE );
		}

		if ( group )
		{
			length -= WIRELESS_MMIE_LENGTH;
		}
		else
		{
			body += WIRELESS_CCMP_IV;
			length -= WIRELESS_CCMP_IV + WIRELESS_CCMP_MIC;
		}

		if ( ( frame[0] == IEEE80211_ACTION ) && ( ! group ) && ( unit->unit_Operational ) &&
			( length >= 28 ) && ( body[0] == IEEE80211_SA_QUERY ) && ( body[1] == 0 ) )
		{
			memset( response, 0, 24 );

			response[0] = IEEE80211_ACTION;
			response[24] = IEEE80211_SA_QUERY;
			response[25] = 1;
			response[26] = body[2];
			response[27] = body[3];

			memcpy( response + 4, unit->unit_Peer, 6 );

			memcpy( response + 10, unit->unit_StationAddress, 6 );

			memcpy( response + 16, unit->unit_Peer, 6 );

			responseLength = 28;

			if ( RT2870_ProtectData( unit, response, & responseLength ) )
			{
				RT2870_TransmitFrame( unit, response, responseLength, unit->unit_Peer, TRUE, NULL );
			}

			return ( unit->unit_BeaconResource == NULL );
		}
	}
	else if ( ( group ) || ( frame[1] & 0x40 ) )
	{
		return ( FALSE );
	}

	if ( ( frame[0] == IEEE80211_DISASSOC ) || ( frame[0] == IEEE80211_DEAUTH ) )
	{
		USBINFO( "RT2870: access point disconnected, reason %lu", (U32) body[0] | ( (U32) body[1] << 8 ) );

		RT2870_DropLink( unit, FALSE );

		return ( TRUE );
	}

	return ( FALSE );
}

// --
