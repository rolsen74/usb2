
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
Wired_DecodeRX( struct devUnit *unit, U8 *buffer, U32 size )
{
	U32 offset;
	U32 length;

	for( offset = 0; ( offset < size ) && ( size - offset >= RTL8152_RX_DESCRIPTOR_SIZE ); )
	{
		length = buffer[offset] | ( (U32) buffer[offset + 1] << 8 );
		length &= RTL8152_RX_LENGTH_MASK;
		offset += RTL8152_RX_DESCRIPTOR_SIZE;

		if ( ( length < 18 ) || ( length > ETHERNET_MTU + 4 ) || ( length > size - offset ) )
		{
			return ( FALSE );
		}

		ReadPacket( unit, buffer + offset, length - 4 );

		offset = ( offset + length + 7 ) & ~7U;
	}

	return ( TRUE );
}

// --
