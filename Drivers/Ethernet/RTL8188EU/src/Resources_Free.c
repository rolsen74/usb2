
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void Resources_Free( void )
{
	USBDEBUG( "RTL8188 : Resources_Free" );

	if ( IRTL8188Timer )
	{
		DropInterface( (PTR) IRTL8188Timer );

		IRTL8188Timer = NULL;
	}

	if ( IUtility )
	{	
		DropInterface( (PTR) IUtility );

		IUtility = NULL;
	}

	if ( UtilityBase )
	{
		CloseLibrary( (PTR) UtilityBase );

		UtilityBase = NULL;
	}

	// --

	if ( IDOS )
	{	
		DropInterface( (PTR) IDOS );

		IDOS = NULL;
	}

	if ( DOSBase )
	{
		CloseLibrary( (PTR) DOSBase );

		DOSBase = NULL;
	}

	// --

	if ( INewlib )
	{	
		DropInterface( INewlib );

		INewlib = NULL;
	}

	if ( NewlibBase )
	{
		CloseLibrary( NewlibBase );

		NewlibBase = NULL;
	}

	// --
}

// --
