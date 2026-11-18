
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
rxdy_gated_en( struct devUnit *unit, int Enable )
{
uint16 val2;

	myRead2( unit, URE_PLA_MISC_1, URE_MCU_TYPE_PLA, & val2 );

	if ( Enable )
	{
		val2 |= URE_RXDY_GATED_EN;
	}
	else
	{
		val2 &= ~URE_RXDY_GATED_EN;
	}

	myWrite2( unit, URE_PLA_MISC_1, URE_MCU_TYPE_PLA, val2 );

}

// --
