
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
RTL8152_Enable_FC( struct devUnit *unit )
{
uint16 val;


	myReadMDIO(  unit, MII_ADVERTISE, & val );

    val |= ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;

    myWriteMDIO( unit, MII_ADVERTISE, val );

}

// --
