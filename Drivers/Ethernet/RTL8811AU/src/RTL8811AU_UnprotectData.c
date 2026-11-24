
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

S32
RTL8811AU_UnprotectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 header )
{
	struct RTL8811AU_Key *key;

	U64 number;
	U32 payload;
	U32 index;
	U32 tid;
	U8 *iv;
	U8 *data;

	if ( ( unit->unit_Encryption != S2ENC_CCMP ) ||
		( *length < header + WIRELESS_CCMP_IV + WIRELESS_CCMP_MIC ) )
	{
		return ( FALSE );
	}

	iv = frame + header;
	index = iv[3] >> 6;

	if ( ( iv[2] ) || ( ( iv[3] & 0x3f ) != 0x20 ) || ( ( frame[4] & 1 ) ? ( ! index ) : ( index != 0 ) ) )
	{
		return ( FALSE );
	}

	key = & unit->unit_Keys[index];

	if ( ! key->Active )
	{
		return ( FALSE );
	}

	tid = ( frame[0] & 0x80 ) ? ( frame[24] & 0x0f ) : WIRELESS_NONQOS;
	number = (U64) iv[0] | ( (U64) iv[1] << 8 ) | ( (U64) iv[4] << 16 ) |
		( (U64) iv[5] << 24 ) | ( (U64) iv[6] << 32 ) | ( (U64) iv[7] << 40 );
	data = iv + WIRELESS_CCMP_IV;
	payload = *length - header - WIRELESS_CCMP_IV - WIRELESS_CCMP_MIC;

	if ( ( number <= key->RxNumber[tid] ) || ( payload > RTL8811AU_MTU + 8 ) ||
		( ! Wireless_CCMP( key->RoundKeys, frame, tid, number, data, payload, data + payload, FALSE ) ) )
	{
		return ( FALSE );
	}

	key->RxNumber[tid] = number;

	memmove( iv, data, payload );

	*length = header + payload;

	return ( TRUE );
}

// --
