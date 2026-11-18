
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
RTL8188EU_ProtectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 type )
{
	struct RTL8188EUKey *key;

	U8 *iv;
	U8 *payload;
	U32 size;
	U32 i;
	U64 number;

	if ( ( unit->unit_RequireOperState ) && ( ! unit->unit_Operational ) &&
		( type != RTL8188EU_EAPOL ) )
	{
		return( FALSE );
	}

	if ( unit->unit_Encryption == S2ENC_NONE )
	{
		return( TRUE );
	}

	key = & unit->unit_Keys[0];

	// EAPOL key exchange must be possible before the pairwise key is active.
	if ( ! key->active )
	{
		return( type == RTL8188EU_EAPOL );
	}

	if ( key->tx_number == RTL8188EU_PACKET_NUMBER_MAX )
	{
		return( FALSE );
	}

	number = ++key->tx_number;
	size = *length - 24;
	iv = frame + 24;
	payload = iv + RTL8188EU_EXT_IV_SIZE;

	memmove( payload, iv, size );

	frame[1] |= 0x40;
	iv[0] = number;
	iv[1] = number >> 8;
	iv[2] = 0;
	iv[3] = 0x20;

	for( i = 2; i < 6; i++ )
	{
		iv[i + 2] = number >> ( i * 8 );
	}

	if ( key->type == S2ENC_TKIP )
	{
		iv[2] = iv[0];
		iv[0] = iv[1];
		iv[1] = ( iv[0] | 0x20 ) & 0x7f;

		RTL8188EU_TKIP( key, frame, RTL8188EU_NONQOS, number, payload, size, TRUE );

		*length += RTL8188EU_EXT_IV_SIZE + RTL8188EU_TKIP_MIC + RTL8188EU_TKIP_ICV;
	}
	else
	{
		RTL8188EU_CCMP( key, frame, RTL8188EU_NONQOS, number, payload, size, payload + size, TRUE );

		*length += RTL8188EU_EXT_IV_SIZE + RTL8188EU_CCMP_MIC;
	}

	return( TRUE );
}

// --
