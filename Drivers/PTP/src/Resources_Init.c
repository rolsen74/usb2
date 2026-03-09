
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32 Resources_Init( struct PTPBase *libBase UNUSED )
{
S32 retval;

	MYDEBUG( "PTP : Resources_Init" );

	retval = FALSE;

	// --
	// Open Resources first
	//
	if ( ! myOpenResources( INT_MAX ))
	{
		MYERROR( "PTP : myOpenResources() failed" );
		goto bailout;
	}

	// --


	// --

	retval = TRUE;

bailout:

	return( retval );
}

// --