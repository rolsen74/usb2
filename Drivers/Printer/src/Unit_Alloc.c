
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

struct PrinterUnit *Unit_Alloc( U32 unitnr )
{
struct PrinterUnit *unit;
S32 error;

	USBDEBUG( "Printer : Unit_Alloc" );

	error = TRUE;

	unit = AllocVecTags( sizeof( struct PrinterUnit ),
		AVT_Clear, 0,
		TAG_END
	);

	if ( ! unit )
	{
		USBERROR( "Printer : Error allocating memory" );
		goto bailout;
	}

	unit->unit_UnitNr = unitnr;
	unit->unit_TaskState = TASK_State_Stopped;

	unit->unit_TaskName = ASPrintf( "Printer unit #%ld", unitnr );

	if ( ! unit->unit_TaskName )
	{
		USBERROR( "Printer : Error allocating memory" );
		goto bailout;
	}

	// --

	// hmm lets not start a Process here.
	// If someone just want to Query the Device type,
	// we don't need too startup every thing for that.

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
