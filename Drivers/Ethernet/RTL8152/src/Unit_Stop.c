
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
unit_Stop( struct devUnit *unit )
{
U32 mask;


	IExec->Forbid();

	if ( ! unit->unit_Process )
	{
		IExec->Permit();

		goto bailout;
	}

	unit->unit_FinishingTask = IExec->FindTask( NULL );

	IExec->SetSignal( 0, SIGBREAKF_CTRL_F );

	IExec->Signal( unit->unit_Process, SIGBREAKF_CTRL_C );

	IExec->Permit();

    while( TRUE )
    {
		mask = IExec->Wait( SIGBREAKF_CTRL_F );

        if ( mask & SIGBREAKF_CTRL_F )
        {
            break;
        }
    }

bailout:

}

// --
