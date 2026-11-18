
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
myWrite2( struct devUnit *unit, U16 reg, U16 index, U32 value )
{
	U32 data;
	U32 shift;

	shift = reg & 2;
	data = LE_SWAP32( ( value & 0xffff ) << ( shift * 8 ) );

	return ( myWriteMem( unit, reg & ~3, index | ( URE_BYTE_EN_WORD << shift ), (PTR) & data, 4 ) );
}

// --
