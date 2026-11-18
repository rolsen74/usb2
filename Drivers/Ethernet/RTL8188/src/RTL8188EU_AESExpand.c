
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
RTL8188EU_AESExpand( U8 *round_keys, const U8 *key )
{
	U32 offset;
	U32 i;
	U32 word;
	U8 rcon;

	rcon = 1;

	memcpy( round_keys, key, 16 );

	for( offset = 16; offset < 176; offset += 16 )
	{
		word = (U32) round_keys[offset - 3] | ( (U32) round_keys[offset - 2] << 8 ) |
			( (U32) round_keys[offset - 1] << 16 ) | ( (U32) round_keys[offset - 4] << 24 );

		word = RTL8188EU_AESSubstitute( word ) ^ rcon;
		rcon = ( rcon << 1 ) ^ ( ( rcon >> 7 ) * 0x1b );

		for( i = 0; i < 4; i++ )
		{
			round_keys[offset + i] = round_keys[offset + i - 16] ^ ( word >> ( i * 8 ) );
		}

		for( ; i < 16; i++ )
		{
			round_keys[offset + i] = round_keys[offset + i - 16] ^ round_keys[offset + i - 4];
		}
	}
}

// --
