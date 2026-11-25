
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <string.h>
#include "WirelessCrypto.h"

// --

static void
DoubleBlock( U8 *block )
{
	U32 i;
	U8 carry;
	U8 next;

	carry = 0;

	for( i = 16; i; i-- )
	{
		next = block[i - 1] >> 7;
		block[i - 1] = ( block[i - 1] << 1 ) | carry;
		carry = next;
	}

	block[15] ^= ( 0U - carry ) & 0x87;
}

// --

void
Wireless_CMAC( const U8 *round_keys, const U8 *data, U32 length, U8 *mac )
{
	U8 subkey[16];
	U32 i;

	memset( mac, 0, 16 );

	memset( subkey, 0, sizeof( subkey ) );

	Wireless_AESBlock( round_keys, subkey );

	DoubleBlock( subkey );

	while( length > 16 )
	{
		for( i = 0; i < 16; i++ )
		{
			mac[i] ^= data[i];
		}

		Wireless_AESBlock( round_keys, mac );

		data += 16;
		length -= 16;
	}

	if ( length < 16 )
	{
		DoubleBlock( subkey );

		mac[length] ^= 0x80;
	}

	for( i = 0; i < length; i++ )
	{
		mac[i] ^= data[i];
	}

	for( i = 0; i < 16; i++ )
	{
		mac[i] ^= subkey[i];
	}

	Wireless_AESBlock( round_keys, mac );
}

// --
