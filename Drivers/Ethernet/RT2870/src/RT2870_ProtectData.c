
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
RT2870_ProtectData( struct EthernetUnit *unit, U8 *frame, U32 *length )
{
	U8 *iv;
	U8 *payload;
	U32 size;
	U32 i;
	U64 number;

	if ( ( ! ( unit->unit_ActiveKeys & RT2870_PAIRWISE_KEY ) ) || ( unit->unit_TxNumber == WIRELESS_PN_MAX ) )
	{
		return ( FALSE );
	}

	number = ++unit->unit_TxNumber;
	size = *length - 24;
	iv = frame + 24;
	payload = iv + WIRELESS_CCMP_IV;

	memmove( payload, iv, size );

	frame[1] |= 0x40;
	iv[0] = number;
	iv[1] = number >> 8;
	iv[2] = 0;
	iv[3] = 0x20;

	for( i = 2; i < 6; i++ )
	{
		iv[i + 2] = number >> ( i * 8 );
	}

	if ( unit->unit_TKIP )
	{
		iv[2] = iv[0];
		iv[0] = iv[1];
		iv[1] = ( iv[0] | 0x20 ) & 0x7f;

		Wireless_TKIP( unit->unit_Keys[0].tkip_key, frame, WIRELESS_NONQOS, number, payload, size, TRUE );

		*length += WIRELESS_CCMP_IV + WIRELESS_TKIP_MIC + WIRELESS_TKIP_ICV;
	}
	else
	{
		Wireless_CCMP( unit->unit_Keys[0].round_keys, frame, ( frame[0] & 0x0c ) ? WIRELESS_NONQOS : WIRELESS_MGMT,
			number, payload, size, payload + size, TRUE );

		*length += WIRELESS_CCMP_IV + WIRELESS_CCMP_MIC;
	}

	return ( TRUE );
}

// --
