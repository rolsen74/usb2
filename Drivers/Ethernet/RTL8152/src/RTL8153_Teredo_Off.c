
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
RTL8153_Teredo_Off( struct devUnit *unit )
{
uint16 val2;

	myRead2(  unit, URE_PLA_TEREDO_CFG,		URE_MCU_TYPE_PLA, & val2 );
	myWrite2( unit, URE_PLA_TEREDO_CFG,		URE_MCU_TYPE_PLA, val2 & ~( URE_TEREDO_SEL | URE_TEREDO_RS_EVENT_MASK | URE_OOB_TEREDO_EN ));

	myWrite2( unit, URE_PLA_WDT6_CTRL,		URE_MCU_TYPE_PLA, URE_WDT6_SET_MODE );
	myWrite2( unit, URE_PLA_REALWOW_TIMER,	URE_MCU_TYPE_PLA, 0 );
	myWrite4( unit, URE_PLA_TEREDO_TIMER,	URE_MCU_TYPE_PLA, 0 );

}

// --
