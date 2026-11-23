
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

extern struct Interface *INewlib;
extern struct Library *NewlibBase;

S32
Resources_Init( void )
{
	S32 retval;

	USBDEBUG( "Printer : Resources_Init" );

	retval = FALSE;

	// --

	NewlibBase = OpenLibrary( "newlib.library", 50 );

	if ( NewlibBase )
	{
		INewlib = GetInterface( NewlibBase, "main", 1, NULL );
	}

	if ( ! INewlib )
	{
		USBERROR( "Error opening Newlib library (v50)" );

		goto bailout;
	}

	// --

	DOSBase = OpenLibrary( "dos.library", 50 );

	if ( DOSBase )
	{
		IDOS = (PTR) GetInterface( (PTR) DOSBase, "main", 1, NULL );
	}

	if ( ! IDOS )
	{
		USBERROR( "Error opening DOS library (v50)" );

		goto bailout;
	}

	// --

	UtilityBase = OpenLibrary( "utility.library", 50 );

	if ( UtilityBase )
	{
		IUtility = (PTR) GetInterface( (PTR) UtilityBase, "main", 1, NULL );
	}

	if ( ! IUtility )
	{
		USBERROR( "Error opening Utility library (v50)" );

		goto bailout;
	}

	retval = TRUE;

bailout:

	return ( retval );
}

// --
