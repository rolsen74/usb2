
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Controller.h"

// --

int
myWrite6( struct devUnit *unit, U16 address, U16 index, const U8 *source )
{
	U8 data[8];

	memset( data, 0, sizeof( data ) );

	memcpy( data, source, 6 );

	return ( myWriteMem( unit, address, index | URE_BYTE_EN_SIX_BYTES, data, sizeof( data ) ) );
}

// --
