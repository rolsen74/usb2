
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void Resources_Free( struct PTPBase *libBase UNUSED )
{
	MYDEBUG( "PTP : Resources_Free" );

	// --


	// --
	// Free Resources last
	myCloseResources();

//bailout:

	return;
}

// --