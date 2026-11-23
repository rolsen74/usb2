
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Resources_Init( void )
{
struct ExecBase *ExecBase;
struct Library *TimerBase;
int retval;


    retval = FALSE;

	// --

    NewlibBase = IExec->OpenLibrary( "newlib.library", 53 );

	if ( NewlibBase )
	{
	INewlib = IExec->GetInterface( NewlibBase, "main", 1, NULL );
	}

    if ( INewlib == NULL )
    {
	    SHOWMSG( "Error opening newlib.library v53" );
	    goto bailout;
    }

	// --

    DOSBase = IExec->OpenLibrary( "dos.library", 53 );

	if ( DOSBase )
	{
	IDOS = (APTR)IExec->GetInterface( DOSBase, "main", 1, NULL );
	}

    if ( IDOS == NULL )
    {
	    SHOWMSG( "Error opening dos.library v53" );
        goto bailout;
    }

	// --

    UtilityBase = IExec->OpenLibrary( "utility.library", 53 );

	if ( UtilityBase )
	{
	IUtility = (APTR)IExec->GetInterface( UtilityBase, "main", 1, NULL );
	}

    if ( IUtility == NULL )
    {
	    SHOWMSG( "Error opening utility.library v53" );
        goto bailout;
    }

	// --

	ExecBase	= (APTR) IExec->Data.LibBase;
	TimerBase	= (APTR) IExec->FindName( & ExecBase->DeviceList, "timer.device" );

	if ( TimerBase )
	{
	ITimer		= (APTR) IExec->GetInterface( TimerBase, "main", 1, NULL );
	}

	if ( ITimer == NULL )
	{
		SHOWMSG( "Error getting Timer Interface" );
		goto bailout;
	}

	// --

    retval = TRUE;

bailout:
    return(	    retval );
}

// --
