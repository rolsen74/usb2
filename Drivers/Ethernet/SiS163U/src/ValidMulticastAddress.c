
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

S32
ValidMulticastAddress( const U8 *address )
{
	static const U8 broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	return ( ( address[0] & 1 ) && memcmp( address, broadcast, sizeof( broadcast ) ) );
}

// --
