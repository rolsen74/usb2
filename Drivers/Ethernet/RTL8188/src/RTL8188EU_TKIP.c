
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static U32
CRC32( const U8 *data, U32 size )
{
	U32 crc;
	U32 i;
	U32 bit;

	crc = 0xffffffff;

	for( i = 0; i < size; i++ )
	{
		crc ^= data[i];

		for( bit = 0; bit < 8; bit++ )
		{
			crc = ( crc >> 1 ) ^ ( 0xedb88320 & ( 0U - ( crc & 1 ) ) );
		}
	}

	return( ~crc );
}

// --

static void
Crypt( const U8 *seed, U8 *data, U32 size )
{
	U8 state[256];
	U8 a;
	U8 b;
	U8 swap;
	U32 i;

	b = 0;

	for( i = 0; i < 256; i++ )
	{
		state[i] = i;
	}

	for( i = 0; i < 256; i++ )
	{
		b += state[i] + seed[i & 15];
		swap = state[i];
		state[i] = state[b];
		state[b] = swap;
	}

	a = 0;
	b = 0;

	for( i = 0; i < size; i++ )
	{
		a++;
		b += state[a];
		swap = state[a];
		state[a] = state[b];
		state[b] = swap;
		data[i] ^= state[(U8) ( state[a] + state[b] )];
	}
}

// --

S32
RTL8188EU_TKIP( const struct RTL8188EUKey *key, const U8 *frame, U32 tid, U64 number,
	U8 *payload, U32 size, S32 encrypt )
{
	U8 addresses[16];
	U8 seed[16];
	U8 mic[RTL8188EU_TKIP_MIC];
	U8 *icv;
	U8 difference;
	U32 crc;
	U32 i;
	U32 length;

	difference = 0;
	length = size + RTL8188EU_TKIP_MIC;
	icv = payload + length;

	RTL8188EU_TKIPSeed( key->tkip_key, frame + 10, number, seed );

	if ( ! encrypt )
	{
		Crypt( seed, payload, length + RTL8188EU_TKIP_ICV );

		crc = CRC32( payload, length );

		for( i = 0; i < RTL8188EU_TKIP_ICV; i++ )
		{
			difference |= icv[i] ^ (U8) ( crc >> ( i * 8 ) );
		}

		if ( difference )
		{
			memset( payload, 0, length );

			return( FALSE );
		}
	}

	memcpy( addresses, frame + ( encrypt ? 16 : 4 ), 6 );
	memcpy( addresses + 6, frame + ( encrypt ? 10 : 16 ), 6 );

	addresses[12] = ( tid == RTL8188EU_NONQOS ) ? 0 : tid;
	addresses[13] = 0;
	addresses[14] = 0;
	addresses[15] = 0;

	RTL8188EU_TKIPMichael( key->tkip_key + RTL8188EU_TEMPORAL_KEY_SIZE + ( encrypt ? 0 : RTL8188EU_TKIP_MIC ), addresses, payload, size, mic );

	if ( encrypt )
	{
		memcpy( payload + size, mic, sizeof( mic ) );

		crc = CRC32( payload, length );

		for( i = 0; i < RTL8188EU_TKIP_ICV; i++ )
		{
			icv[i] = crc >> ( i * 8 );
		}

		Crypt( seed, payload, length + RTL8188EU_TKIP_ICV );
	}
	else
	{
		for( i = 0; i < RTL8188EU_TKIP_MIC; i++ )
		{
			difference |= mic[i] ^ payload[size + i];
		}

		if ( difference )
		{
			memset( payload, 0, length );

			// A valid ICV with a bad Michael MIC triggers countermeasures.
			return( -1 );
		}
	}

	return( TRUE );
}

// --
