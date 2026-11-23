
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
Wireless_Frame_Header( struct Wireless *wireless, U8 *frame, U8 subtype )
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

S32
Wireless_Reply_From_AP( struct Wireless *wireless, const U8 *frame, U32 length, U8 subtype )
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
	if ( ( ! Wireless_Reply_From_AP( wireless, frame, length, 0xb0 ) ) ||
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
	if ( ! Wireless_Reply_From_AP( wireless, frame, length, 0x10 ) )
	{
		return ( FALSE );
	}

	wireless->Status = frame[26] | ( (U16) frame[27] << 8 );
	wireless->AssocID = ( frame[28] | ( (U16) frame[29] << 8 ) ) & 0x3fff;

	return ( TRUE );
}

// --

enum WirelessResult
Wireless_Authenticate( struct Wireless *wireless )
{
	enum WirelessResult result;

	U32 attempt;
	U32 limit;
	U32 timeout;
	U8 frame[WIRELESS_80211_HEADER + 6];

	limit = wireless->ScanRT2870 ? 3 : 1;
	timeout = wireless->ScanRT2870 ? 2000000 : 1500000;
	result = WIRELESS_TIMEOUT;

	for( attempt = 0; attempt < limit; attempt++ )
	{
		Wireless_Frame_Header( wireless, frame, 0xb0 );

		memset( frame + WIRELESS_80211_HEADER, 0, 6 );

		frame[26] = 1;
		wireless->Status = 0;

		if ( attempt )
		{
			printf( "Retrying authentication (%u/%u).\n", (unsigned) ( attempt + 1 ), (unsigned) limit );
		}

		result = Wireless_Wait_Frame( wireless, timeout, frame, sizeof( frame ), Match_Authentication, FALSE );

		if ( result != WIRELESS_TIMEOUT )
		{
			break;
		}
	}

	if ( result == WIRELESS_TIMEOUT )
	{
		fprintf( stderr, "Authentication response timed out.\n" );
	}
	else if ( ( result == WIRELESS_MATCH ) && ( wireless->Status ) )
	{
		fprintf( stderr, "Authentication rejected by the access point (status %u).\n", (unsigned) wireless->Status );

		return ( WIRELESS_IO_ERROR );
	}

	return ( result );
}

// --

enum WirelessResult
Wireless_Associate( struct Wireless *wireless, const U8 *associationIE, U32 associationLength )
{
	enum WirelessResult result;

	U32 length;
	U32 rates;
	U8 frame[WIRELESS_80211_HEADER + 4 + 2 + WIRELESS_MAX_SSID + 4 +
		sizeof( wireless->AP.Rates ) + USB2WPA3_ASSOC_IE_MAX];

	Wireless_Frame_Header( wireless, frame, 0x00 );

	frame[24] = 1;  // Infrastructure station, open system
	frame[25] = 0;
	frame[26] = 10; // Listen interval in beacon intervals
	frame[27] = 0;
	frame[28] = 0;
	frame[29] = wireless->SSIDLength;

	memcpy( frame + 30, wireless->SSID, wireless->SSIDLength );

	length = 30 + wireless->SSIDLength;
	rates = wireless->AP.RateCount > 8 ? 8 : wireless->AP.RateCount;
	frame[length++] = 1;
	frame[length++] = rates;

	memcpy( frame + length, wireless->AP.Rates, rates );

	length += rates;

	if ( wireless->AP.RateCount > rates )
	{
		frame[length++] = 50;
		frame[length++] = wireless->AP.RateCount - rates;

		memcpy( frame + length, wireless->AP.Rates + rates, wireless->AP.RateCount - rates );

		length += wireless->AP.RateCount - rates;
	}

	if ( ( associationIE ) && ( associationLength ) )
	{
		memcpy( frame + length, associationIE, associationLength );

		length += associationLength;
	}

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

	return ( WIRELESS_MATCH );

failed:

	if ( result == WIRELESS_TIMEOUT )
	{
		fprintf( stderr, "Association response timed out.\n" );
	}
	else if ( result == WIRELESS_MATCH )
	{
		fprintf( stderr, "Association rejected by the access point (status %u).\n", (unsigned) wireless->Status );

		return ( WIRELESS_IO_ERROR );
	}

	return ( result );
}

// --

enum WirelessResult
Wireless_Connect( struct Wireless *wireless, U32 channel )
{
	enum WirelessResult result;

	const U8 *associationIE;
	U32 cipher;
	U32 associationLength;

	cipher = ( wireless->WPA2 || wireless->UseSAE ) ? S2ENC_CCMP : S2ENC_NONE;

	if ( wireless->UseWPA )
	{
		cipher = S2ENC_TKIP;
	}

	if ( ! Wireless_Set_Options( wireless, channel, wireless->AP.BSSID, 0, cipher, FALSE ) )
	{
		return ( WIRELESS_IO_ERROR );
	}

	result = wireless->UseSAE ? WPA3_Authenticate( wireless ) : Wireless_Authenticate( wireless );

	if ( result != WIRELESS_MATCH )
	{
		return ( result );
	}

	associationLength = 0;

	if ( wireless->UseSAE )
	{
		associationIE = wireless->IWPA3->AssociationIEs( wireless->WPA3, & associationLength );

		if ( ( ! associationIE ) || ( ! associationLength ) || ( associationLength > USB2WPA3_ASSOC_IE_MAX ) )
		{
			fprintf( stderr, "Could not prepare WPA3 association information.\n" );

			return ( WIRELESS_IO_ERROR );
		}
	}
	else if ( wireless->UseWPA )
	{
		associationIE = wireless->IWPA->AssociationIE();
		associationLength = USB2WPA_ASSOC_IE_SIZE;
	}
	else
	{
		associationIE = wireless->WPA2 ? wireless->IWPA2->AssociationIE() : NULL;
		associationLength = associationIE ? USB2WPA2_ASSOC_IE_SIZE : 0;
	}

	result = Wireless_Associate( wireless, associationIE, associationLength );

	if ( result != WIRELESS_MATCH )
	{
		return ( result );
	}

	return ( Wireless_Set_Options( wireless, channel, wireless->AP.BSSID, wireless->AssocID, cipher,
		cipher == S2ENC_NONE ) ? WIRELESS_MATCH : WIRELESS_IO_ERROR );
}
