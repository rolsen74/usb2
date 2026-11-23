
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct Library *SysBase;
struct ExecIFace *IExec;
struct DOSIFace *IDOS;
struct Library *DOSBase;
struct UtilityIFace *IUtility;
struct Library *UtilityBase;

// --

S32
Resources_Init( void )
{
	DOSBase = OpenLibrary( "dos.library", 52 );

	if ( DOSBase )
	{
		IDOS = (PTR) GetInterface( DOSBase, "main", 1, NULL );
	}

	if ( ! IDOS )
	{
		return ( FALSE );
	}

	UtilityBase = OpenLibrary( "utility.library", 50 );

	if ( UtilityBase )
	{
		IUtility = (PTR) GetInterface( UtilityBase, "main", 1, NULL );
	}

	return ( IUtility != NULL );
}

// --
