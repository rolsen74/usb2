
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <string.h>
#include "BluetoothTest.h"

// --

static LONG
HexDigit( char digit )
{
	if ( ( digit >= '0' ) && ( digit <= '9' ) )
	{
		return ( digit - '0' );
	}

	digit |= 0x20;

	if ( ( digit >= 'a' ) && ( digit <= 'f' ) )
	{
		return ( digit - 'a' + 10 );
	}

	return ( -1 );
}

// --

LONG
ParseAddress( const char *text, uint8_t *address )
{
	ULONG byte;
	LONG high;
	LONG low;

	if ( strlen( text ) != 17 )
	{
		return ( FALSE );
	}

	for( byte = 0; byte < 6; byte++ )
	{
		high = HexDigit( text[byte * 3] );

		low = HexDigit( text[byte * 3 + 1] );

		if ( ( high < 0 ) || ( low < 0 ) || ( ( byte < 5 ) && ( text[byte * 3 + 2] != ':' ) ) )
		{
			return ( FALSE );
		}

		address[byte] = ( high << 4 ) | low;
	}

	return ( TRUE );
}

// --
