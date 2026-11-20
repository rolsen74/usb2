
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <exec/types.h>
#include "WirelessCrypto.h"

// --

S32
Wireless_InstallIGTK( struct WirelessIGTK *keys, U32 index, const U8 *key, const U8 *ipn )
{
	struct WirelessIGTK *slot;

	U32 candidate;
	U32 i;
	U8 difference;

	if ( ( index < WIRELESS_IGTK_FIRST ) || ( index >= WIRELESS_IGTK_FIRST + WIRELESS_IGTK_COUNT ) )
	{
		return ( FALSE );
	}

	for( candidate = 0; candidate < WIRELESS_IGTK_COUNT; candidate++ )
	{
		slot = & keys[candidate];

		if ( slot->active )
		{
			difference = 0;

			for( i = 0; i < 16; i++ )
			{
				difference |= slot->round_keys[i] ^ key[i];
			}

			if ( ! difference )
			{
				// Preserve replay state when the same key is delivered again.
				return ( candidate == index - WIRELESS_IGTK_FIRST );
			}
		}
	}

	slot = & keys[index - WIRELESS_IGTK_FIRST];
	slot->ipn = 0;

	for( i = 0; i < 6; i++ )
	{
		slot->ipn |= (U64) ipn[i] << ( 8 * i );
	}

	Wireless_AESExpand( slot->round_keys, key );

	slot->active = TRUE;

	return ( TRUE );
}

// --
