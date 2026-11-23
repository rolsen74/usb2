
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
RTL8821CU_SameAddress( const U8 *left, const U8 *right )
{
	U32 index;

	for( index = 0; index < RTL8821CU_ADDR_SIZE; index++ )
	{
		if ( left[index] != right[index] )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --
