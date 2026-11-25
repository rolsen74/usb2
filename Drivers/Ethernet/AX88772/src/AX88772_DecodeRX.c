
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define AX_LENGTH_MASK 0x07ff
#define ETHERNET_HEADER_SIZE 14

// --

void
AX88772_ResetRX( struct AX88772Rx *rx )
{
	rx->Length = 0;
	rx->Used = 0;
	rx->HeaderUsed = 0;
	rx->Padding = 0;
}

// --

static S32
ReadHeader( const U8 *header )
{
	U16 length;
	U16 inverse;

	length = ( header[0] | ( (U16) header[1] << 8 ) ) & AX_LENGTH_MASK;
	inverse = ( header[2] | ( (U16) header[3] << 8 ) ) & AX_LENGTH_MASK;

	if ( ( length ^ inverse ) != AX_LENGTH_MASK )
	{
		return ( -1 );
	}

	if ( ( length ) && ( ( length < ETHERNET_HEADER_SIZE ) || ( length > ETHERNET_MTU ) ) )
	{
		return ( -1 );
	}

	return ( length );
}

// --

S32
Wired_DecodeRX( struct devUnit *unit, U8 *buffer, U32 size )
{
	struct AX88772Rx *rx;

	U32 amount;
	U32 next;
	S32 length;
	S32 valid;

	rx = & unit->unit_AXReceive;
	valid = TRUE;

	if ( rx->Length )
	{
		next = rx->Length - rx->Used + ( rx->Length & 1 );

		// A missing chunk must not turn the next good frame into a continuation.
		if ( ( size >= next + sizeof( rx->Header ) ) && ( ReadHeader( buffer + next ) < 0 ) )
		{
			AX88772_ResetRX( rx );

			valid = FALSE;
		}
	}

	while( size )
	{
		if ( rx->Padding )
		{
			buffer++;
			size--;
			rx->Padding = 0;

			continue;
		}

		if ( ! rx->Length )
		{
			amount = MIN( size, sizeof( rx->Header ) - rx->HeaderUsed );

			memcpy( rx->Header + rx->HeaderUsed, buffer, amount );

			rx->HeaderUsed += amount;
			buffer += amount;
			size -= amount;

			if ( rx->HeaderUsed < sizeof( rx->Header ) )
			{
				break;
			}

			length = ReadHeader( rx->Header );

			if ( length < 0 )
			{
				AX88772_ResetRX( rx );

				return ( FALSE );
			}

			rx->HeaderUsed = 0;
			rx->Length = length;

			// A zero-length, complemented header terminates a USB packet.
			continue;
		}

		amount = MIN( size, (U32) ( rx->Length - rx->Used ) );

		memcpy( rx->Frame + rx->Used, buffer, amount );

		rx->Used += amount;
		buffer += amount;
		size -= amount;

		if ( rx->Used == rx->Length )
		{
			ReadPacket( unit, rx->Frame, rx->Length );

			rx->Padding = rx->Length & 1;
			rx->Length = 0;
			rx->Used = 0;
		}
	}

	return ( valid );
}

// --
