
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

// Returns the PTP string length in bytes, including its count byte.
U32
_ptp_Read_Name( const U8 *data, U32 remaining, STR name, U32 size )
{
	U32 bytes;
	U32 index;
	U32 used;
	U16 code;

	name[0] = 0;
	used = 0;

	if ( ! remaining )
	{
		return ( 0 );
	}

	bytes = 1 + 2 * data[0];

	if ( bytes == 1 )
	{
		return ( bytes );
	}

	if ( ( bytes > remaining ) || ( data[bytes - 1] ) || ( data[bytes - 2] ) )
	{
		return ( 0 );
	}

	for( index = 1; ( index < bytes - 2 ) && ( used < size - 1 ); index += 2 )
	{
		code = data[index] | ( (U16) data[index + 1] << 8 );

		if ( ( code == ' ' ) && ( ! used ) )
		{
			continue;
		}

		if ( ( code < 32 ) || ( ( code >= 127 ) && ( code < 160 ) ) || ( code >= 255 ) ||
			( code == '/' ) || ( code == ':' ) || ( code == '*' ) || ( code == '"' ) )
		{
			code = '_';
		}

		name[used++] = code;
	}

	while( ( used ) && ( name[used - 1] == ' ' ) )
	{
		used--;
	}

	name[used] = 0;

	return ( bytes );
}

// --
