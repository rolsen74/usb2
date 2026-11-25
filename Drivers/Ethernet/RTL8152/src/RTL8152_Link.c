
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Controller.h"

// --

#define RTL8152_MII_STATUS 1U
#define RTL8152_MII_LINK 4U

S32
Wired_PHYLink( struct devUnit *unit )
{
	U16 status;
	S32 error;

	// The status bit latches low.
	unit->unit_USBControlError = 0;

	error = myReadMDIO( unit, RTL8152_MII_STATUS, & status );

	if ( ! error )
	{
		error = myReadMDIO( unit, RTL8152_MII_STATUS, & status );
	}

	unit->unit_USBControlError = 0;

	return ( ( ! error ) && ( status & RTL8152_MII_LINK ) );
}

// --
