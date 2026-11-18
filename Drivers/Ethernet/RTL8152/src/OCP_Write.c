
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
myWriteReg( struct devUnit *unit, uint16 addr, uint16 data )
{
uint16 reg;
int error;

	SHOWMSG( "Addr: %04lx", addr );

	error = myWrite2( unit, URE_PLA_OCP_GPHY_BASE, URE_MCU_TYPE_PLA, addr & 0xf000 );

	reg = ( addr & 0x0fff ) | 0xb000;

	error |= myWrite2( unit, reg, URE_MCU_TYPE_PLA, data );
	return(		error );
}

// --
