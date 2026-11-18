
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

void
myTallyReset( struct devUnit *unit )
{
uint16 val2;

	myRead2(  unit, URE_PLA_RSTTALLY, URE_MCU_TYPE_PLA, & val2 );

	val2 |= URE_TALLY_RESET;
	
	myWrite2( unit, URE_PLA_RSTTALLY, URE_MCU_TYPE_PLA, val2 );

}

// --
