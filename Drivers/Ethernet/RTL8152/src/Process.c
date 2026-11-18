
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
unit_Process( void )
{
struct devUnit *unit;
struct Task *finish;
struct Task *self;

	//--------

	self = IExec->FindTask( NULL );

	unit = (APTR) ( self -> tc_UserData );

	finish = unit->unit_FinishingTask;

	//--------

	if ( unit_Process_Init( unit ))
	{
		unit->unit_Process = self;
		unit->unit_Running = TRUE;

		IExec->Signal( finish, SIGBREAKF_CTRL_E );

		// --

		unit_Process_MainLoop( unit );

		finish = NULL;
	}

	unit_Process_Free( unit );

	IExec->Forbid();

	if ( ! finish )
	{
		finish = unit->unit_FinishingTask;
	}

	unit->unit_Process = NULL;

	if ( finish )
	{
		IExec->Signal( finish, SIGBREAKF_CTRL_F );
	}
}

// --
