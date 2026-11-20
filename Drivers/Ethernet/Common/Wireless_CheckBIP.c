
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <string.h>
#include <exec/types.h>
#include "WirelessCrypto.h"

// --

#define IEEE80211_MMIE 76
#define WIRELESS_BIP_MAX_FRAME 2304

// --

S32
Wireless_CheckBIP( struct WirelessIGTK *keys, const U8 *frame, U32 length )
{
	struct WirelessIGTK *key;

	const U8 *mmie;
	U8 data[WIRELESS_BIP_MAX_FRAME];
	U8 mac[16];
	U8 difference;
	U32 i;
	U64 number;

	if ( ( length < 26 + WIRELESS_MMIE_LENGTH ) || ( length > WIRELESS_BIP_MAX_FRAME ) )
	{
		return ( FALSE );
	}

	mmie = frame + length - WIRELESS_MMIE_LENGTH;

	if ( ( mmie[0] != IEEE80211_MMIE ) || ( mmie[1] != WIRELESS_MMIE_LENGTH - 2 ) ||
		( mmie[3] ) || ( mmie[2] < WIRELESS_IGTK_FIRST ) || ( mmie[2] >= WIRELESS_IGTK_FIRST + WIRELESS_IGTK_COUNT ) )
	{
		return ( FALSE );
	}

	key = & keys[mmie[2] - WIRELESS_IGTK_FIRST];
	number = 0;

	for( i = 0; i < 6; i++ )
	{
		number |= (U64) mmie[i + 4] << ( 8 * i );
	}

	if ( ( ! key->active ) || ( number <= key->ipn ) )
	{
		return ( FALSE );
	}

	data[0] = frame[0];
	data[1] = frame[1] & 0xc7;

	memcpy( data + 2, frame + 4, 18 );

	memcpy( data + 20, frame + 24, length - 24 - 8 );

	memset( data + length - 4 - 8, 0, 8 );

	Wireless_CMAC( key->round_keys, data, length - 4, mac );

	difference = 0;

	for( i = 0; i < 8; i++ )
	{
		difference |= mac[i] ^ mmie[10 + i];
	}

	if ( difference )
	{
		return ( FALSE );
	}

	key->ipn = number;

	return ( TRUE );
}

// --
