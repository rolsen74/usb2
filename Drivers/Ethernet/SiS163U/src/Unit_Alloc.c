
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct EthernetUnit *
Unit_Alloc( U32 unitnr )
{
	struct EthernetUnit *unit;

	unit = AllocVecTags( sizeof( *unit ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! unit )
	{
		return ( NULL );
	}

	unit->unit_UnitNr = unitnr;
	unit->unit_TaskState = TASK_State_Stopped;

	InitSemaphore( & unit->unit_CommandLock );

	NewList( & unit->unit_Pending );

	NewList( & unit->unit_Multicast );

	return ( unit );
}

// --
