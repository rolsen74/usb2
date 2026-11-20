
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct Library *DOSBase;
struct DOSIFace *IDOS;
struct Library *UtilityBase;
struct UtilityIFace *IUtility;
struct Library *NewlibBase;
struct Interface *INewlib;

// --

S32
Resources_Init( struct EthernetBase *ethBase )
{
	InitSemaphore( & ethBase->eth_ETH_Semaphore );

	NewlibBase = OpenLibrary( "newlib.library", 50 );

	if ( ! NewlibBase )
	{
		return ( FALSE );
	}

	INewlib = GetInterface( NewlibBase, "main", 1, NULL );

	if ( ! INewlib )
	{
		return ( FALSE );
	}

	DOSBase = OpenLibrary( "dos.library", 50 );

	if ( ! DOSBase )
	{
		return ( FALSE );
	}

	IDOS = (PTR) GetInterface( DOSBase, "main", 1, NULL );

	if ( ! IDOS )
	{
		return ( FALSE );
	}

	UtilityBase = OpenLibrary( "utility.library", 50 );

	if ( ! UtilityBase )
	{
		return ( FALSE );
	}

	IUtility = (PTR) GetInterface( UtilityBase, "main", 1, NULL );

	return ( IUtility != NULL );
}

// --
