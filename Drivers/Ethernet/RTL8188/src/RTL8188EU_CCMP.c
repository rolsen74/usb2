
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
RTL8188EU_CCMP( const struct RTL8188EUKey *key, const U8 *frame, U32 tid, U64 number,
	U8 *payload, U32 length, U8 *mic, S32 encrypt )
{
	U8 aad[32];
	U8 counter[16];
	U8 auth[16];
	U8 stream[16];
	U8 plain;
	U8 difference;
	U32 offset;
	U32 count;
	U32 bytes;
	U32 i;

	// CCM associated data authenticates the fixed header fields and QoS TID.
	memset( aad, 0, sizeof( aad ) );

	aad[1] = ( tid == RTL8188EU_NONQOS ) ? 22 : 24;
	aad[2] = frame[0] & 0x8f;
	aad[3] = frame[1] & 0xc7;

	memcpy( aad + 4, frame + 4, 18 );

	aad[22] = frame[22] & 15;
	aad[24] = ( tid == RTL8188EU_NONQOS ) ? 0 : tid;
	counter[0] = 1;
	counter[1] = aad[24];

	memcpy( counter + 2, frame + 10, 6 );

	for( i = 0; i < 6; i++ )
	{
		counter[8 + i] = number >> ( ( 5 - i ) * 8 );
	}

	counter[14] = 0;
	counter[15] = 0;

	memcpy( auth, counter, sizeof( auth ) );

	auth[0] = 0x59;
	auth[14] = length >> 8;
	auth[15] = length;

	RTL8188EU_AESBlock( key->round_keys, auth );

	for( offset = 0; offset < sizeof( aad ); offset += 16 )
	{
		for( i = 0; i < 16; i++ )
		{
			auth[i] ^= aad[offset + i];
		}

		RTL8188EU_AESBlock( key->round_keys, auth );
	}

	for( offset = 0, count = 1; offset < length; offset += bytes, count++ )
	{
		bytes = length - offset;

		if ( bytes > 16 )
		{
			bytes = 16;
		}

		memcpy( stream, counter, sizeof( stream ) );

		stream[14] = count >> 8;
		stream[15] = count;

		RTL8188EU_AESBlock( key->round_keys, stream );

		for( i = 0; i < bytes; i++ )
		{
			plain = payload[offset + i];

			if ( ! encrypt )
			{
				plain ^= stream[i];
			}

			auth[i] ^= plain;
			payload[offset + i] ^= stream[i];
		}

		RTL8188EU_AESBlock( key->round_keys, auth );
	}

	RTL8188EU_AESBlock( key->round_keys, counter );

	difference = 0;

	for( i = 0; i < RTL8188EU_CCMP_MIC; i++ )
	{
		if ( encrypt )
		{
			mic[i] = auth[i] ^ counter[i];
		}
		else
		{
			difference |= mic[i] ^ auth[i] ^ counter[i];
		}
	}

	if ( difference )
	{
		memset( payload, 0, length );

		return( FALSE );
	}

	return( TRUE );
}

// --
