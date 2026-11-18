
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
RTL_RX_VLan_En( struct devUnit *unit, int Enable )
{
uint16 val;

  
    myRead2( unit, URE_USB_UPS_CTRL, URE_MCU_TYPE_USB, & val );

    if ( Enable )
    {
	    val |= URE_CPCR_RX_VLAN;
    }
    else
    {
	    val &= ~URE_CPCR_RX_VLAN;
    }

    myWrite2( unit, URE_USB_UPS_CTRL, URE_MCU_TYPE_USB, val );

}

// --
