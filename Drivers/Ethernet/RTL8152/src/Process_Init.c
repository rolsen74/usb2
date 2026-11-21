
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
unit_Process_Init( struct devUnit *unit )
{
int retval;


    retval = FALSE;

	if ( unit_Process_InitMisc( unit ) == FALSE )
	{
		IExec->DebugPrintF( "RTL8152: could not initialize unit resources\n" );
		goto bailout;
	}

	if ( unit_Process_OpenUSBDevice( unit ) == FALSE )
	{
		IExec->DebugPrintF( "RTL8152: could not open usb2.device\n" );
		goto bailout;
	}

	if ( unit_Process_FindUSBDevice( unit ) == FALSE )
	{
		IExec->DebugPrintF( "RTL8152: no 0BDA:8152 vendor-specific interface found\n" );
		goto bailout;
	}

	if ( unit_Process_InitUSB( unit ) == FALSE )
	{
		IExec->DebugPrintF( "RTL8152: could not claim USB interface or allocate transfers\n" );
		goto bailout;
	}

	if ( Driver_Setup( unit ) == FALSE )
	{
		IExec->DebugPrintF( "RTL8152: chip setup failed\n" );
		goto bailout;
	}

	IExec->DebugPrintF( "RTL8152: %s unit %lu initialized\n", VERS, unit->unit_UnitNumber );

    retval = TRUE;

bailout:
    return(	    retval );
}

// --
