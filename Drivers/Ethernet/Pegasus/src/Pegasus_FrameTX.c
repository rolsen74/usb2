
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define ETHERNET_MIN_FRAME 60

U32
Wired_FrameTX( struct devUnit *unit, U8 *buffer, U32 length )
{
	U32 total;

	if ( length < ETHERNET_MIN_FRAME )
	{
		memset( buffer + WIRED_TX_HEADER_SIZE + length, 0, ETHERNET_MIN_FRAME - length );

		length = ETHERNET_MIN_FRAME;
	}

	buffer[0] = length;
	buffer[1] = length >> 8;
	total = length + WIRED_TX_HEADER_SIZE;

	// The chip uses a short USB packet to terminate a frame.
	if ( total % unit->unit_USBBulkOut->EndPointMax == 0 )
	{
		buffer[total] = 0;
		total++;
	}

	if ( ! unit->unit_PegasusTXStarted )
	{
		unit->unit_PegasusTXStarted = TRUE;

		IExec->DebugPrintF( "Pegasus: queued first bulk OUT, %lu bytes\n", total );
	}

	return ( total );
}

// --
