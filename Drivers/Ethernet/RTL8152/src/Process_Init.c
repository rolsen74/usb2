
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Version.h"

// --

int
unit_Process_Init( struct devUnit *unit, struct USB2_DriverMessage *msg )
{
int retval;


    retval = FALSE;

	if ( unit_Process_InitMisc( unit ) == FALSE )
	{
		IExec->DebugPrintF( "RTL8152: could not initialize unit resources\n" );
		goto bailout;
	}

	if ( unit_Process_InitUSB( unit, msg ) == FALSE )
	{
		IExec->DebugPrintF( "RTL8152: could not claim USB interface or allocate transfers\n" );
		goto bailout;
	}

	if ( Driver_Setup( unit ) == FALSE )
	{
		IExec->DebugPrintF( "RTL8152: chip setup failed\n" );
		goto bailout;
	}

	IExec->DebugPrintF( "RTL8152: %s initialized\n", VERS );

    retval = TRUE;

bailout:
    return(	    retval );
}

// --
