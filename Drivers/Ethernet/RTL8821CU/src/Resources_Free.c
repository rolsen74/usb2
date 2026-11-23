
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
	if ( IUtility )
	{
		DropInterface( (PTR) IUtility );

		IUtility = NULL;
	}

	if ( UtilityBase )
	{
		CloseLibrary( UtilityBase );

		UtilityBase = NULL;
	}

	if ( IDOS )
	{
		DropInterface( (PTR) IDOS );

		IDOS = NULL;
	}

	if ( DOSBase )
	{
		CloseLibrary( DOSBase );

		DOSBase = NULL;
	}
}

// --
