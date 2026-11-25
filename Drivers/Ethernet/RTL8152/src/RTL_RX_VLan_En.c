
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
	U16 reg;
	U16 mask;
	U16 val;

	reg = URE_PLA_CPCR;
	mask = URE_CPCR_RX_VLAN;

	if ( RTL8153B_CHIP( unit ) )
	{
		reg = URE_PLA_RCR + 2;
		mask = URE_RX_VLAN_8153B;
	}

	if ( ! myRead2( unit, reg, URE_MCU_TYPE_PLA, & val ) )
	{
		val = ( val & ~mask ) | ( Enable ? mask : 0 );

		myWrite2( unit, reg, URE_MCU_TYPE_PLA, val );
	}

}

// --
