
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
Wired_FrameTX( struct devUnit *unit, U8 *buffer, U32 length )
{
	U32 total;

	buffer[0] = length;
	buffer[1] = length >> 8;
	buffer[2] = ~buffer[0];
	buffer[3] = ~buffer[1];
	total = length + WIRED_TX_HEADER_SIZE;

	// Terminate a max-packet multiple without requiring a USB zero-length packet.
	if ( total % unit->unit_USBBulkOut->EndPointMax == 0 )
	{
		buffer[total++] = 0;
		buffer[total++] = 0;
		buffer[total++] = 0xff;
		buffer[total++] = 0xff;
	}

	return ( total );
}

// --
