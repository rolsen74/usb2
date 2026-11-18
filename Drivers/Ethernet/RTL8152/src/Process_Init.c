
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
unit_Process_Init( struct devUnit *unit )
{
int retval;


    retval = FALSE;

	if ( unit_Process_InitMisc( unit ) == FALSE )
	{
		SHOWMSG( "Init Misc Failed" );
		goto bailout;
	}

	if ( unit_Process_OpenUSBDevice( unit ) == FALSE )
	{
		SHOWMSG( "Error Opening USB Device" );
		goto bailout;
	}

	if ( unit_Process_FindUSBDevice( unit ) == FALSE )
	{
		SHOWMSG( "Error Finding USB Device" );
		goto bailout;
	}

	if ( unit_Process_InitUSB( unit ) == FALSE )
	{
		SHOWMSG( "Error Allocating USB Resources" );
		goto bailout;
	}

	if ( Driver_Setup( unit ) == FALSE )
	{
		SHOWMSG( "Error Setting up Chip" );
		goto bailout;
	}

    retval = TRUE;

bailout:
    return(	    retval );
}

// --
