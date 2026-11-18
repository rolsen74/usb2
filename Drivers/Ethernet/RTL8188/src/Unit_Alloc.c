
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct EthernetUnit *Unit_Alloc( U32 unitnr )
{
struct EthernetUnit *unit;
S32 error;

	USBDEBUG( "RTL8188 : Unit_Alloc" );

	error = TRUE;

	unit = AllocVecTags( sizeof( struct EthernetUnit ),
		AVT_ClearWithValue, 0,
		TAG_END
	);

	if ( ! unit )
	{
		USBERROR( "RTL8188 : Error allocating memory" );
		goto bailout;
	}

	unit->unit_UnitNr = unitnr;
	unit->unit_TaskState = TASK_State_Stopped;

	InitSemaphore( & unit->unit_CommandLock );

	NewList( & unit->unit_Pending );

	NewList( & unit->unit_Multicast );

	unit->unit_TaskName = ASPrintf( "RTL8188 unit #%ld", unitnr );

	if ( ! unit->unit_TaskName )
	{
		USBERROR( "RTL8188 : Error allocating memory" );
		goto bailout;
	}

	// --

	error = FALSE;

bailout:

	if (( unit ) && ( error ))
	{
		Unit_Free( unit );
		unit = NULL;
	}

	return( unit );
}

// --
