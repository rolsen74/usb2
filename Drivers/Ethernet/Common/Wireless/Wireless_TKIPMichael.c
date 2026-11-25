
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <exec/types.h>
#include <string.h>
#include "WirelessCrypto.h"

// --

static U32
ReadLE32( const U8 *data )
{
	return( data[0] | ( (U32) data[1] << 8 ) | ( (U32) data[2] << 16 ) | ( (U32) data[3] << 24 ) );
}

// --

void
Wireless_TKIPMichael( const U8 *key, const U8 *addresses, const U8 *payload, U32 size, U8 *mic )
{
	U32 left;
	U32 right;
	U32 words;
	U32 offset;
	U32 word;
	U32 i;
	U32 j;

	left = ReadLE32( key );
	right = ReadLE32( key + 4 );
	words = 4 + size / 4;

	for( i = 0; i < words + 2; i++ )
	{
		word = 0;

		if ( i < 4 )
		{
			word = ReadLE32( addresses + i * 4 );
		}
		else if ( i < words )
		{
			word = ReadLE32( payload + ( i - 4 ) * 4 );
		}
		else if ( i == words )
		{
			offset = size & ~3U;

			for( j = 0; j < ( size & 3 ); j++ )
			{
				word |= (U32) payload[offset + j] << ( j * 8 );
			}

			word |= (U32) 0x5a << ( j * 8 );
		}

		left ^= word;
		right ^= ( left << 17 ) | ( left >> 15 );
		left += right;
		right ^= ( ( left & 0x00ff00ff ) << 8 ) | ( ( left & 0xff00ff00 ) >> 8 );
		left += right;
		right ^= ( left << 3 ) | ( left >> 29 );
		left += right;
		right ^= ( left >> 2 ) | ( left << 30 );
		left += right;
	}

	for( i = 0; i < 4; i++ )
	{
		mic[i] = left >> ( i * 8 );
		mic[i + 4] = right >> ( i * 8 );
	}
}

// --
