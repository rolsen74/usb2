
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
myWriteMMD( struct devUnit *unit, uint16 dev, uint16 reg, uint16 data )
{
int error;

	error  = myWriteReg( unit, URE_OCP_EEE_AR,	 FUN_ADDR | dev );
	error |= myWriteReg( unit, URE_OCP_EEE_DATA, reg );
	error |= myWriteReg( unit, URE_OCP_EEE_AR,	 FUN_DATA | dev );

	error |= myWriteReg( unit, URE_OCP_EEE_DATA, data );

	error |= myWriteReg( unit, URE_OCP_EEE_AR,	 0x0000 );
	return(		error );
}

// --
