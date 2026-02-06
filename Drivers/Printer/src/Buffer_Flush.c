
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void Buffer_Flush( struct PrinterUnit *unit )
{
struct PrtBuffer *pb;

	pb = unit->unit_Buffer_Active;

	USBERROR( "Buffer_Flush : PB %p :", pb );

	if ( pb )
	{
		unit->unit_Buffer_Active = NULL;

		AddTail( unit->unit_Buffer_WaitingList, & pb->pb_Node );
		unit->unit_Buffer_WaitingCount++;

		Buffer_Schedule( unit );
	}
}

// --
