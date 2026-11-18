
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

int
unit_Start( struct devUnit *unit )
{
U32 mask;
int retval;


    retval  = FALSE;

	unit->unit_FinishingTask = IExec->FindTask( NULL );

	IExec->SetSignal( 0, SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F );

	if ( IDOS->CreateNewProcTags(
		NP_Name,        "RTL8152 Unit Process",
	    NP_Child,       FALSE,
        NP_Priority,    10,
        NP_StackSize,   20*1024,
		NP_Entry,       unit_Process,
		NP_UserData,    unit,
        TAG_END ))
    {
        while( TRUE )
        {
			mask = IExec->Wait( SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F );

        	// CTRL+E is meaning every thing is okey
	        if ( mask & SIGBREAKF_CTRL_E )
            {
		        retval = TRUE;
                break;
            }

        	// CTRL+F is meaning init failed
	        if ( mask & SIGBREAKF_CTRL_F )
            {
                break;
            }
        }
    }

	unit->unit_FinishingTask = NULL;
    return(     retval );
}

// --
