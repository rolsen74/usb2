
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

int
myWriteMem( struct devUnit *unit, U16 address, U16 index, U8 *data, U16 length )
{
	return ( RTL8152_Control( unit, CMD_WRITE, address, index, data, length ) );
}

// --
