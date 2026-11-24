
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

static S32
IsHandshakeReply( const U8 *frame, U32 length, U32 type )
{
	return ( ( type == RTL8811AU_EAPOL ) && ( length == 131 ) && ( frame[33] == 3 ) &&
		( frame[34] == 0 ) && ( frame[35] == 95 ) && ( frame[36] == 2 ) &&
		( frame[37] == 3 ) && ( frame[38] == 0x0a ) && ( frame[129] == 0 ) && ( frame[130] == 0 ) );
}

// --

S32
RTL8811AU_ProtectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 type )
{
	struct RTL8811AU_Key *key;

	U64 number;
	U32 payload;
	U8 *iv;
	U8 *data;

	if ( unit->unit_Encryption == S2ENC_NONE )
	{
		return ( TRUE );
	}

	key = & unit->unit_Keys[0];

	if ( ( ! key->Active ) || ( IsHandshakeReply( frame, *length, type ) ) )
	{
		return ( type == RTL8811AU_EAPOL );
	}

	if ( key->TxNumber == WIRELESS_PN_MAX )
	{
		return ( FALSE );
	}

	number = ++key->TxNumber;
	iv = frame + 24;
	data = iv + WIRELESS_CCMP_IV;
	payload = *length - 24;

	memmove( data, iv, payload );

	frame[1] |= 0x40;
	iv[0] = number;
	iv[1] = number >> 8;
	iv[2] = 0;
	iv[3] = 0x20;
	iv[4] = number >> 16;
	iv[5] = number >> 24;
	iv[6] = number >> 32;
	iv[7] = number >> 40;

	if ( ! Wireless_CCMP( key->RoundKeys, frame, WIRELESS_NONQOS, number, data, payload, data + payload, TRUE ) )
	{
		return ( FALSE );
	}

	*length += WIRELESS_CCMP_IV + WIRELESS_CCMP_MIC;

	return ( TRUE );
}

// --
