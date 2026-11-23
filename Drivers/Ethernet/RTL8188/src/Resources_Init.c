
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
struct TimerIFace *IRTL8188Timer;

S32 Resources_Init( void )
{
struct ExecBase *exec;
struct Library *timer;

S32 retval;

	USBDEBUG( "RTL8188 : Resources_Init" );

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

	// --

	exec = (PTR) IExec->Data.LibBase;
	timer = (PTR) FindName( & exec->DeviceList, "timer.device" );

	if ( timer )
	{
		IRTL8188Timer = (PTR) GetInterface( timer, "main", 1, NULL );
	}

	retval = TRUE;

bailout:

	return( retval );
}

// --
