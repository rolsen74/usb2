
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
Process_Init( struct EthernetUnit *unit )
{
	if ( ! Process_Find_Device( unit ) )
	{
		return ( FALSE );
	}

	return ( Process_Setup_Device( unit ) );
}

// --
