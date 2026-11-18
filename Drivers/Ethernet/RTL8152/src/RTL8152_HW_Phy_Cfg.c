
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
RTL8152_HW_Phy_Cfg( struct devUnit *unit )
{
uint16 val2;

	myReadMDIO( unit, MII_BMCR, & val2 );

	if ( val2 & BMCR_PDOWN )
	{
		val2 &= ~BMCR_PDOWN;

		myWriteMDIO( unit, MII_BMCR, val2 );
	}


}

// --
