
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

U32
Wired_FrameTX( struct devUnit *unit UNUSED, U8 *buffer, U32 length )
{
	U32 options;

	options = length | RTL8152_TX_SEGMENTS;
	buffer[0] = options;
	buffer[1] = options >> 8;
	buffer[2] = options >> 16;
	buffer[3] = options >> 24;

	memset( buffer + 4, 0, 4 );

	return ( length + WIRED_TX_HEADER_SIZE );
}

// --
