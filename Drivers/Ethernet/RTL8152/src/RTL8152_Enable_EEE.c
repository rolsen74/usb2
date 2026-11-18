
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
RTL8152_Enable_EEE( struct devUnit *unit )
{
    RTL8152_EEE_Enable( unit, TRUE );

	myWriteMMD( unit, MDIO_MMD_AN, MDIO_AN_EEE_ADV, MDIO_EEE_100TX );
}

// --
