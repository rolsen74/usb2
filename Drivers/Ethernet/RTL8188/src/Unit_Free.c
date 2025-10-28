
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

void Unit_Free( struct EthernetUnit *unit )
{
	USBDEBUG( "RTL8188 : Unit_Free :" );

	if ( ! unit )
	{
		goto bailout;
	}

	unit->unit_StartupComplete = FALSE;

	// --

	if ( unit->unit_TaskAdr )
	{
		while( unit->unit_TaskState == TASK_State_Starting )
		{
			Delay( 1 );	
		}

		unit->unit_ExitMask = 0;	// unused at the moment
		unit->unit_ExitParent = FindTask( NULL );
		Signal( unit->unit_TaskAdr, SIGBREAKF_CTRL_C );

		while( unit->unit_TaskState != TASK_State_Stopped )
		{
			Delay( 1 );	
		}
	}

	// --
	
	if ( unit->unit_TaskName )
	{
		FreeVec( unit->unit_TaskName );
	}

	// --

	FreeVec( unit );

bailout:

	return;
}

// --
