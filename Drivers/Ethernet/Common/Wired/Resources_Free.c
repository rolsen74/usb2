
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
Resources_Free( void )
{
	// --

	SHOWMSG( "Drop Timer Interface" );

	if ( ITimer )
    {
		IExec->DropInterface( (APTR) ITimer );
		ITimer = NULL;
    }

    SHOWMSG( "Drop Utility Interface" );

    if ( IUtility )
    {
		IExec->DropInterface( (APTR) IUtility );
	    IUtility = NULL;
    }

    SHOWMSG( "Close Utility Library" );

    if ( UtilityBase )
    {
	    IExec->CloseLibrary( UtilityBase );
	    UtilityBase = NULL;
    }

	// --

    SHOWMSG( "Drop DOS Interface" );

    if ( IDOS )
    {
		IExec->DropInterface( (APTR) IDOS );
	    IDOS = NULL;
    }

    SHOWMSG( "Close DOS Library" );

    if ( DOSBase )
    {
	    IExec->CloseLibrary( DOSBase );
	    DOSBase = NULL;
    }

	// --

    SHOWMSG( "Drop Newlib Interface" );

    if ( INewlib )
    {
	    IExec->DropInterface( INewlib );
	    INewlib = NULL;
    }

    SHOWMSG( "Close Newlib Library" );

    if ( NewlibBase )
    {
	    IExec->CloseLibrary( NewlibBase );
	    NewlibBase = NULL;
    }

}

// --
