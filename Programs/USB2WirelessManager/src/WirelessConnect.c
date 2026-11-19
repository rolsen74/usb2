
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
Frame_Header( struct Wireless *wireless, U8 *frame, U8 subtype )
{
	memset( frame, 0, WIRELESS_80211_HEADER );

	frame[0] = subtype;
	frame[22] = wireless->Sequence << 4;
	frame[23] = wireless->Sequence >> 4;
	wireless->Sequence = ( wireless->Sequence + 1 ) & 0x0fff;

	memcpy( frame + 4, wireless->AP.BSSID, WIRELESS_ADDR_SIZE );

	memcpy( frame + 10, wireless->Station, WIRELESS_ADDR_SIZE );

	memcpy( frame + 16, wireless->AP.BSSID, WIRELESS_ADDR_SIZE );
}

// --

static S32
Reply_From_AP( struct Wireless *wireless, const U8 *frame, U32 length, U8 subtype )
{
	return ( ( length >= 30 ) && ( frame[0] == subtype ) && ( ! ( frame[1] & 0x43 ) ) &&
		( ! memcmp( frame + 4, wireless->Station, WIRELESS_ADDR_SIZE ) ) &&
		( ! memcmp( frame + 10, wireless->AP.BSSID, WIRELESS_ADDR_SIZE ) ) &&
		( ! memcmp( frame + 16, wireless->AP.BSSID, WIRELESS_ADDR_SIZE ) ) );
}

// --

static S32
Match_Authentication( struct Wireless *wireless, const U8 *frame, U32 length )
{
	if ( ( ! Reply_From_AP( wireless, frame, length, 0xb0 ) ) ||
		( frame[24] ) || ( frame[25] ) || ( frame[26] != 2 ) || ( frame[27] ) )
	{
		return ( FALSE );
	}

	wireless->Status = frame[28] | ( (U16) frame[29] << 8 );

	return ( TRUE );
}

// --

static S32
Match_Association( struct Wireless *wireless, const U8 *frame, U32 length )
{
	if ( ! Reply_From_AP( wireless, frame, length, 0x10 ) )
	{
		return ( FALSE );
	}

	wireless->Status = frame[26] | ( (U16) frame[27] << 8 );
	wireless->AssocID = ( frame[28] | ( (U16) frame[29] << 8 ) ) & 0x3fff;

	return ( TRUE );
}

// --

enum WirelessResult
Wireless_Connect( struct Wireless *wireless, U32 channel )
{
	enum WirelessResult result;

	const char *stage;
	U32 length;
	U32 cipher;
	U8 frame[WIRELESS_80211_HEADER + 4 + 2 + WIRELESS_MAX_SSID + 2 +
		sizeof( wireless->AP.Rates ) + USB2WPA2_ASSOC_IE_SIZE];

	cipher = wireless->WPA2 ? S2ENC_CCMP : S2ENC_NONE;

	if ( ! Wireless_Set_Options( wireless, channel, wireless->AP.BSSID, 0, cipher, FALSE ) )
	{
		return ( WIRELESS_IO_ERROR );
	}

	Frame_Header( wireless, frame, 0xb0 );

	memset( frame + WIRELESS_80211_HEADER, 0, 6 );

	frame[26] = 1;
	stage = "Authentication";

	result = Wireless_Wait_Frame( wireless, 1500000, frame, 30, Match_Authentication, FALSE );

	if ( ( result != WIRELESS_MATCH ) || ( wireless->Status ) )
	{
		goto failed;
	}

	Frame_Header( wireless, frame, 0x00 );

	frame[24] = 1;  // Infrastructure station, open system
	frame[25] = 0;
	frame[26] = 10; // Listen interval in beacon intervals
	frame[27] = 0;
	frame[28] = 0;
	frame[29] = wireless->SSIDLength;

	memcpy( frame + 30, wireless->SSID, wireless->SSIDLength );

	length = 30 + wireless->SSIDLength;
	frame[length++] = 1;
	frame[length++] = wireless->AP.RateCount;

	memcpy( frame + length, wireless->AP.Rates, wireless->AP.RateCount );

	length += wireless->AP.RateCount;

	if ( wireless->WPA2 )
	{
		memcpy( frame + length, wireless->IWPA2->AssociationIE(), USB2WPA2_ASSOC_IE_SIZE );

		length += USB2WPA2_ASSOC_IE_SIZE;
	}

	stage = "Association";

	result = Wireless_Wait_Frame( wireless, 1500000, frame, length, Match_Association, FALSE );

	if ( ( result != WIRELESS_MATCH ) || ( wireless->Status ) )
	{
		goto failed;
	}

	if ( ( ! wireless->AssocID ) || ( wireless->AssocID > 2007 ) )
	{
		fprintf( stderr, "Access point returned an invalid association ID.\n" );

		return ( WIRELESS_IO_ERROR );
	}

	return ( Wireless_Set_Options( wireless, channel, wireless->AP.BSSID, wireless->AssocID, cipher,
		! wireless->WPA2 ) ?
		WIRELESS_MATCH : WIRELESS_IO_ERROR );

failed:

	if ( result == WIRELESS_TIMEOUT )
	{
		fprintf( stderr, "%s response timed out.\n", stage );
	}
	else if ( result == WIRELESS_MATCH )
	{
		fprintf( stderr, "%s rejected by the access point (status %u).\n", stage, (unsigned) wireless->Status );

		return ( WIRELESS_IO_ERROR );
	}

	return ( result );
}
