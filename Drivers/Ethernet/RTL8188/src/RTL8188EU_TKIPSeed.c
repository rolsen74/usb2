
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static U16
Substitute( U16 word )
{
	U32 bytes;
	U32 doubled;
	U16 a;
	U16 b;

	bytes = RTL8188EU_AESSubstitute( word );
	doubled = ( ( bytes << 1 ) & 0xfefefefe ) ^ ( ( ( bytes >> 7 ) & 0x01010101 ) * 0x1b );
	a = ( ( doubled & 255 ) << 8 ) | ( ( doubled ^ bytes ) & 255 );
	b = ( doubled & 0xff00 ) | ( ( ( doubled ^ bytes ) >> 8 ) & 255 );

	return( a ^ (U16) ( ( b >> 8 ) | ( b << 8 ) ) );
}

// --

static U16
RotateRight( U16 value )
{
	return( ( value >> 1 ) | ( value << 15 ) );
}

// --

void
RTL8188EU_TKIPSeed( const U8 *key, const U8 *sender, U64 number, U8 *seed )
{
	U16 temporal[8];
	U16 mixed[6];
	U32 i;
	U32 j;

	for( i = 0; i < 8; i++ )
	{
		temporal[i] = key[2 * i] | ( (U16) key[2 * i + 1] << 8 );
	}

	mixed[0] = number >> 16;
	mixed[1] = number >> 32;

	for( i = 0; i < 3; i++ )
	{
		mixed[i + 2] = sender[2 * i] | ( (U16) sender[2 * i + 1] << 8 );
	}

	// Phase 1 combines the transmitter address, upper TSC and temporal key.
	for( i = 0; i < 8; i++ )
	{
		j = i & 1;
		mixed[0] += Substitute( mixed[4] ^ temporal[j] );
		mixed[1] += Substitute( mixed[0] ^ temporal[j + 2] );
		mixed[2] += Substitute( mixed[1] ^ temporal[j + 4] );
		mixed[3] += Substitute( mixed[2] ^ temporal[j + 6] );
		mixed[4] += Substitute( mixed[3] ^ temporal[j] ) + i;
	}

	// Phase 2 folds in the lower TSC to produce the RC4 key for this MPDU.
	mixed[5] = mixed[4] + (U16) number;
	mixed[0] += Substitute( mixed[5] ^ temporal[0] );

	for( i = 1; i < 6; i++ )
	{
		mixed[i] += Substitute( mixed[i - 1] ^ temporal[i] );
	}

	mixed[0] += RotateRight( mixed[5] ^ temporal[6] );
	mixed[1] += RotateRight( mixed[0] ^ temporal[7] );

	for( i = 2; i < 6; i++ )
	{
		mixed[i] += RotateRight( mixed[i - 1] );
	}

	seed[0] = number >> 8;
	seed[1] = ( seed[0] | 0x20 ) & 0x7f;
	seed[2] = number;
	seed[3] = ( mixed[5] ^ temporal[0] ) >> 1;

	for( i = 0; i < 6; i++ )
	{
		seed[4 + i * 2] = mixed[i];
		seed[5 + i * 2] = mixed[i] >> 8;
	}
}

// --
