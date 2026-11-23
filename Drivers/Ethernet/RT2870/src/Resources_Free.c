
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
	if ( IRT2870Timer )
	{
		DropInterface( (PTR) IRT2870Timer );

		IRT2870Timer = NULL;
	}

	if ( IUtility )
	{
		DropInterface( (PTR) IUtility );
	}

	if ( UtilityBase )
	{
		CloseLibrary( UtilityBase );
	}

	if ( IDOS )
	{
		DropInterface( (PTR) IDOS );
	}

	if ( DOSBase )
	{
		CloseLibrary( DOSBase );
	}

	if ( INewlib )
	{
		DropInterface( INewlib );
	}

	if ( NewlibBase )
	{
		CloseLibrary( NewlibBase );
	}
}

// --
