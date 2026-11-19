
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static const U8 CCMP[] = { 0, 0x0f, 0xac, 4 };
static const U8 PSK[] = { 0, 0x0f, 0xac, 2 };

const U8 AssociationRSN[USB2WPA2_ASSOC_IE_SIZE] =
{
	48, 20, 1, 0,
	0, 0x0f, 0xac, 4, // Group cipher
	1, 0, 0, 0x0f, 0xac, 4, // Pairwise cipher
	1, 0, 0, 0x0f, 0xac, 2, // PSK
	0, 0 // RSN capabilities
};

// --

static S32
Contains_Suite( const U8 **data, U32 *length, const U8 *suite )
{
	U32 count;
	S32 found;

	if ( *length < 2 )
	{
		return ( FALSE );
	}

	count = ( *data )[0] | ( (U32) ( *data )[1] << 8 );
	*data += 2;
	*length -= 2;

	if ( count > *length / 4 )
	{
		return ( FALSE );
	}

	found = FALSE;

	while( count-- )
	{
		found |= memcmp( *data, suite, 4 ) == 0;

		*data += 4;
		*length -= 4;
	}

	return ( found );
}

// --

S32
RSN_Supports_WPA2( const U8 *data, U32 length )
{
	if ( ( length < 8 ) || ( data[0] != 1 ) || ( data[1] ) || ( memcmp( data + 2, CCMP, sizeof( CCMP ) ) ) )
	{
		return ( FALSE );
	}

	data += 6;
	length -= 6;

	if ( ( ! Contains_Suite( & data, & length, CCMP ) ) || ( ! Contains_Suite( & data, & length, PSK ) ) )
	{
		return ( FALSE );
	}

	// Required protected management frames need a different AKM and IGTK support.
	return ( ( ! length ) || ( ( length >= 2 ) && ( ! ( data[0] & 0x40 ) ) ) );
}
