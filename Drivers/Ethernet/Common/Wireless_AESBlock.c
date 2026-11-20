
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

void
Wireless_AESBlock( const U8 *round_keys, U8 *block )
{
	U8 next[16];
	U32 round;
	U32 column;
	U32 word;
	U32 rotated;
	U32 combined;
	U32 i;

	for( i = 0; i < 16; i++ )
	{
		block[i] ^= round_keys[i];
	}

	for( round = 1; round <= 10; round++ )
	{
		for( column = 0; column < 4; column++ )
		{
			// Select ShiftRows input, then substitute all four bytes in parallel.
			word = (U32) block[column * 4] |
				( (U32) block[( ( column + 1 ) & 3 ) * 4 + 1] << 8 ) |
				( (U32) block[( ( column + 2 ) & 3 ) * 4 + 2] << 16 ) |
				( (U32) block[( ( column + 3 ) & 3 ) * 4 + 3] << 24 );

			word = Wireless_AESSubstitute( word );

			if ( round < 10 )
			{
				rotated = ( word >> 8 ) | ( word << 24 );
				combined = word ^ rotated;

				// MixColumns uses the column XOR and twice each adjacent byte XOR.
				word ^= combined ^ ( ( combined >> 16 ) | ( combined << 16 ) ) ^
					( ( combined << 1 ) & 0xfefefefe ) ^
					( ( ( combined >> 7 ) & 0x01010101 ) * 0x1b );
			}

			for( i = 0; i < 4; i++ )
			{
				next[column * 4 + i] = ( word >> ( i * 8 ) ) ^ round_keys[round * 16 + column * 4 + i];
			}
		}

		memcpy( block, next, sizeof( next ) );
	}
}

// --
