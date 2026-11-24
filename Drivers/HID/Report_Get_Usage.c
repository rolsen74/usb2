
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_CODE U32
Report_Get_Usage( struct HID_BitNode *bn, U32 index, U32 repeatLast )
{
	struct HID_UsageNode *un;

	U32 last;
	U64 count;

	last = 0;

	for ( un = Header_Head( &bn->UsageHeader ); un; un = Node_Next( un ) )
	{
		count = (U64) un->Max - un->Min + 1;

		if ( index < count )
		{
			return ( un->Min + index );
		}

		index -= count;
		last = un->Max;
	}

	return ( repeatLast ? last : 0 );
}

// --
