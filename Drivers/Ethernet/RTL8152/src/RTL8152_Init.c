
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
RTL8152_Init( struct devUnit *unit )
{
U32 val4;
uint16 val2;

	RTL8152_Disable_ALDPS( unit );

	if ( unit->unit_ChipVersion == URE_CHIP_VER_4C00 )
	{
		myRead2(  unit, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA, & val2 );
		myWrite2( unit, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA, val2 & ~URE_LED_MODE_MASK );
	}

	RTL8152_Power_Cut( unit, FALSE );

	myRead2(  unit, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA, & val2 );
	myWrite2( unit, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA, val2 | URE_TX_10M_IDLE_EN | URE_PFM_PWM_SWITCH );

	myRead4(  unit, URE_PLA_MAC_PWR_CTRL, URE_MCU_TYPE_PLA, & val4 );
	val4 &= ~URE_MCU_CLK_RATIO_MASK;
	val4 |=  URE_MCU_CLK_RATIO | URE_D3_CLK_GATED_EN;
	myWrite4( unit, URE_PLA_MAC_PWR_CTRL, URE_MCU_TYPE_PLA, val4 );

	myWrite2( unit, URE_PLA_GPHY_INTR_IMR, URE_MCU_TYPE_PLA,
		URE_GPHY_STS_MSK | URE_SPEED_DOWN_MSK | URE_SPDWN_RXDV_MSK | URE_SPDWN_LINKCHG_MSK );

	RTL8152_Enable_EEE( unit );
	RTL8152_Enable_ALDPS( unit );
	RTL8152_Enable_FC( unit );

	myTallyReset( unit );

	myRead2(  unit, URE_USB_USB_CTRL, URE_MCU_TYPE_USB, & val2 );
	myWrite2( unit, URE_USB_USB_CTRL, URE_MCU_TYPE_USB, val2 & ~( URE_RX_AGG_DISABLE | URE_RX_ZERO_EN ));

}

// --
