
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
ValidStationAddress( const U8 *address )
{
	U32 index;

	if ( address[0] & 1 )
	{
		return ( FALSE );
	}

	for( index = 0; index < RTL8821CU_ADDR_SIZE; index++ )
	{
		if ( address[index] )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --
