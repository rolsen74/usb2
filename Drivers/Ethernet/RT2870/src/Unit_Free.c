
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
Unit_Free( struct EthernetUnit *unit )
{
	USBDEBUG( "RT2870 : Unit_Free :" );

	if ( ! unit )
	{
		goto bailout;
	}

	// --

	Forbid();

	if ( unit->unit_TaskAdr )
	{
		Signal( unit->unit_TaskAdr, SIGBREAKF_CTRL_C );
	}

	Permit();

	while( unit->unit_TaskState != TASK_State_Stopped )
	{
		Delay( 1 );
	}

	// --

	FreeVec( unit );

bailout:

	return;
}

// --
