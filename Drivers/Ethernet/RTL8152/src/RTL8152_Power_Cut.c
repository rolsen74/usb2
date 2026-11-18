
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
RTL8152_Power_Cut( struct devUnit *unit, int Enable )
{
uint16 val2;

	// --

	myRead2(  unit, URE_USB_UPS_CTRL, URE_MCU_TYPE_USB, & val2 );

	if ( Enable )
	{
		val2 |= URE_POWER_CUT;
	}
	else
	{
		val2 &= ~URE_POWER_CUT;
	}

	myWrite2( unit, URE_USB_UPS_CTRL, URE_MCU_TYPE_USB, val2 );

	// --

	myRead2(  unit, URE_USB_PM_CTRL_STATUS, URE_MCU_TYPE_USB, & val2 );

	val2 &= ~URE_RESUME_INDICATE;

	myWrite2( unit, URE_USB_PM_CTRL_STATUS, URE_MCU_TYPE_USB, val2 );

	// --

}

// --
