
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

void Resources_Free( struct PrinterBase *devBase )
{
	USBDEBUG( "Printer : Resources_Free" );

	// --



	// --

	if ( IUtility )
	{	
		DropInterface( (PTR) IUtility );
	}

	if ( UtilityBase )
	{
		CloseLibrary( (PTR) UtilityBase );
	}

	// --

	if ( IDOS )
	{	
		DropInterface( (PTR) IDOS );
	}

	if ( DOSBase )
	{
		CloseLibrary( (PTR) DOSBase );
	}

	// --

	if ( INewlib )
	{	
		DropInterface( INewlib );
	}

	if ( NewlibBase )
	{
		CloseLibrary( NewlibBase );
	}

	// --
}

// --
