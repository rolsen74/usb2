
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
myWriteMDIO( struct devUnit *unit, uint16 reg_addr, uint16 data )
{
int error;

	error = myWriteReg( unit, URE_OCP_BASE_MII + reg_addr * 2, data );
	return(		error );
}

// --
