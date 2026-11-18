
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
RTL8188EU_UnprotectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 *header )
{
	struct RTL8188EUKey *key;

	U8 *iv;
	U8 *payload;
	U32 index;
	U32 trailer;
	S32 status;
	U32 size;
	U32 tid;
	U32 i;
	U64 number;

	if ( ( unit->unit_Encryption == S2ENC_NONE ) ||
		( *length < *header + RTL8188EU_EXT_IV_SIZE + RTL8188EU_CCMP_MIC + 8 ) )
	{
		return( FALSE );
	}

	iv = frame + *header;
	index = iv[3] >> 6;

	if ( ( ( iv[3] & 0x3f ) != 0x20 ) ||
		( ( index != 0 ) != ( ( frame[4] & 1 ) != 0 ) ) )
	{
		return( FALSE );
	}

	key = & unit->unit_Keys[index];

	if ( ! key->active )
	{
		return( FALSE );
	}

	tid = ( frame[0] & 0x80 ) ? frame[24] & 15 : RTL8188EU_NONQOS;
	trailer = RTL8188EU_CCMP_MIC;

	if ( key->type == S2ENC_TKIP )
	{
		trailer = RTL8188EU_TKIP_MIC + RTL8188EU_TKIP_ICV;

		if ( ( iv[1] != ( ( iv[0] | 0x20 ) & 0x7f ) ) ||
			( *length < *header + RTL8188EU_EXT_IV_SIZE + trailer + 8 ) ||
			( RTL8188EU_TKIPTime() < unit->unit_TKIPBlockedUntil ) )
		{
			return( FALSE );
		}

		number = iv[2] | ( (U64) iv[0] << 8 );
	}
	else
	{
		if ( iv[2] )
		{
			return( FALSE );
		}

		number = iv[0] | ( (U64) iv[1] << 8 );
	}

	for( i = 2; i < 6; i++ )
	{
		number |= (U64) iv[i + 2] << ( i * 8 );
	}

	if ( number <= key->rx_number[tid] )
	{
		return( FALSE );
	}

	payload = iv + RTL8188EU_EXT_IV_SIZE;
	size = *length - *header - RTL8188EU_EXT_IV_SIZE - trailer;

	if ( size > RTL8188EU_MTU + 8 )
	{
		return( FALSE );
	}

	if ( key->type == S2ENC_TKIP )
	{
		status = RTL8188EU_TKIP( key, frame, tid, number, payload, size, FALSE );

		if ( status < 0 )
		{
			RTL8188EU_TKIPFailure( unit );
		}
	}
	else
	{
		status = RTL8188EU_CCMP( key, frame, tid, number, payload, size, payload + size, FALSE );
	}

	if ( status <= 0 )
	{
		return( FALSE );
	}

	key->rx_number[tid] = number;
	*header += RTL8188EU_EXT_IV_SIZE;
	*length -= trailer;

	return( TRUE );
}

// --
