
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

#define RTL8153_READY_POLLS	100

// --

static void
UpdateUSBWord( struct devUnit *unit, U16 reg, U16 clear, U16 set )
{
	U16 value;

	if ( ! myRead2( unit, reg, URE_MCU_TYPE_USB, & value ) )
	{
		myWrite2( unit, reg, URE_MCU_TYPE_USB, ( value & ~clear ) | set );
	}
}

// --

static S32
WaitAutoload( struct devUnit *unit )
{
	U32 count;
	U16 value;

	for( count = 0; count < RTL8153_READY_POLLS; count++ )
	{
		if ( myRead2( unit, URE_PLA_BOOT_CTRL, URE_MCU_TYPE_PLA, & value ) )
		{
			return ( FALSE );
		}

		if ( value & URE_AUTOLOAD_DONE )
		{
			return ( TRUE );
		}

		IDOS->Delay( 1 );
	}

	IExec->DebugPrintF( "RTL8152: Gigabit chip autoload timed out\n" );

	return ( FALSE );
}

// --

static S32
WaitPHY( struct devUnit *unit, S32 accept_inactive )
{
	U32 count;
	U16 value;

	for( count = 0; count < RTL8153_READY_POLLS; count++ )
	{
		if ( myReadReg( unit, URE_OCP_PHY_STATUS, & value ) )
		{
			return ( FALSE );
		}

		value &= URE_PHY_STAT_MASK;

		if ( ( value == URE_PHY_STAT_LAN_ON ) ||
			( ( accept_inactive ) && ( ( value == URE_PHY_STAT_PWRDN ) || ( value == URE_PHY_STAT_EXT_INIT ) ) ) )
		{
			return ( TRUE );
		}

		IDOS->Delay( 1 );
	}

	IExec->DebugPrintF( "RTL8152: Gigabit PHY state $%04lx timed out\n", (U32) value );

	return ( FALSE );
}

// --

static void
WriteSRAM( struct devUnit *unit, U16 addr, U16 value )
{
	myWriteReg( unit, URE_OCP_SRAM_ADDR, addr );

	myWriteReg( unit, URE_OCP_SRAM_DATA, value );
}

// --

static void
ConfigureRTL8153( struct devUnit *unit )
{
	U16 value;
	U16 burst;
	U8 byte;
	U8 timer;

	timer = URE_LPM_TIMER_500US;

	if ( unit->unit_ChipVersion == URE_CHIP_VER_5C10 )
	{
		UpdateUSBWord( unit, URE_USB_SSPHYLINK2, URE_PWD_DN_SCALE_MASK, URE_PWD_DN_SCALE( 96 ) );

		myRead1( unit, URE_USB_USB2PHY, URE_MCU_TYPE_USB, & byte );

		myWrite1( unit, URE_USB_USB2PHY, URE_MCU_TYPE_USB, byte | URE_USB2PHY_L1 | URE_USB2PHY_SUSPEND );

		// Use the longer LPM timeout on USB 2.
		timer = URE_LPM_TIMER_500MS;
	}

	if ( unit->unit_ChipVersion == URE_CHIP_VER_5C20 )
	{
		myRead1( unit, URE_PLA_DMY_REG0, URE_MCU_TYPE_PLA, & byte );

		myWrite1( unit, URE_PLA_DMY_REG0, URE_MCU_TYPE_PLA, byte & ~URE_ECM_ALDPS );
	}

	if ( unit->unit_ChipVersion & ( URE_CHIP_VER_5C20 | URE_CHIP_VER_5C30 ) )
	{
		myRead2( unit, URE_USB_BURST_SIZE, URE_MCU_TYPE_USB, & burst );

		myRead1( unit, URE_USB_CSR_DUMMY1, URE_MCU_TYPE_USB, & byte );

		byte &= ~URE_DYNAMIC_BURST;
		byte |= burst ? URE_DYNAMIC_BURST : 0;

		myWrite1( unit, URE_USB_CSR_DUMMY1, URE_MCU_TYPE_USB, byte );
	}

	myRead1( unit, URE_USB_CSR_DUMMY2, URE_MCU_TYPE_USB, & byte );

	myWrite1( unit, URE_USB_CSR_DUMMY2, URE_MCU_TYPE_USB, byte | URE_EP4_FULL_FC );

	UpdateUSBWord( unit, URE_USB_WDT11_CTRL, URE_TIMER11_EN, 0 );

	myRead2( unit, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA, & value );

	myWrite2( unit, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA, value & ~URE_LED_MODE_MASK );

	myWrite1( unit, URE_USB_LPM_CTRL, URE_MCU_TYPE_USB, timer | URE_FIFO_EMPTY_1FB | URE_ROK_EXIT_LPM );

	UpdateUSBWord( unit, URE_USB_AFE_CTRL2, URE_SEN_VAL_MASK, URE_SEN_VAL_NORMAL | URE_SEL_RXIDLE );

	myWrite2( unit, URE_USB_CONNECT_TIMER, URE_MCU_TYPE_USB, 1 );

	myWrite2( unit, URE_PLA_MAC_PWR_CTRL, URE_MCU_TYPE_PLA, URE_ALDPS_SPDWN_RATIO );

	myWrite2( unit, URE_PLA_MAC_PWR_CTRL2, URE_MCU_TYPE_PLA, URE_EEE_SPDWN_RATIO );

	myWrite2( unit, URE_PLA_MAC_PWR_CTRL3, URE_MCU_TYPE_PLA,
		URE_PKT_AVAIL_SPDWN_EN | URE_SUSPEND_SPDWN_EN | URE_U1U2_SPDWN_EN | URE_L1_SPDWN_EN );

	myWrite2( unit, URE_PLA_MAC_PWR_CTRL4, URE_MCU_TYPE_PLA, URE_PWRSAVE_SPDWN_EN | URE_RXDV_SPDWN_EN |
		URE_TX10MIDLE_EN | URE_TP100_SPDWN_EN | URE_TP500_SPDWN_EN | URE_TP1000_SPDWN_EN | URE_EEE_SPDWN_EN );

	if ( unit->unit_ChipVersion != URE_CHIP_VER_5C30 )
	{
		myWriteReg( unit, URE_OCP_ADC_CFG, URE_CKADSEL_L | URE_ADC_EN | URE_EN_EMI_L );
	}

	if ( unit->unit_ChipVersion == URE_CHIP_VER_5C00 )
	{
		myReadReg( unit, URE_OCP_EEE_CFG, & value );

		myWriteReg( unit, URE_OCP_EEE_CFG, value & ~URE_CTAP_SHORT_EN );
	}

	myReadReg( unit, URE_OCP_POWER_CFG, & value );

	myWriteReg( unit, URE_OCP_POWER_CFG, ( value & ~URE_EN_ALDPS ) | URE_EEE_CLKDIV_EN | URE_EN_10M_PLLOFF );

	myReadReg( unit, URE_OCP_DOWN_SPEED, & value );

	myWriteReg( unit, URE_OCP_DOWN_SPEED, value | URE_EN_10M_BGOFF );

	WriteSRAM( unit, URE_SRAM_IMPEDANCE, 0x0b13 );

	myRead2( unit, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA, & value );

	myWrite2( unit, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA, value | URE_PFM_PWM_SWITCH );

	WriteSRAM( unit, URE_SRAM_LPF_CFG, 0xf70f );

	WriteSRAM( unit, URE_SRAM_10M_AMP1, 0x00af );

	WriteSRAM( unit, URE_SRAM_10M_AMP2, 0x0208 );
}

// --

static void
ConfigureRTL8153B( struct devUnit *unit )
{
	U16 value;
	U8 byte;

	myWrite2( unit, URE_USB_MSC_TIMER, URE_MCU_TYPE_USB, 0x0fff );

	myWrite2( unit, URE_USB_U1U2_TIMER, URE_MCU_TYPE_USB, 500 );

	// Clear pending runtime wake state left by firmware or a previous driver.
	myRead1( unit, URE_PLA_INDICATE_FLAG, URE_MCU_TYPE_PLA, & byte );

	myWrite1( unit, URE_PLA_INDICATE_FLAG, URE_MCU_TYPE_PLA, byte & ~URE_UPCOMING_RUNTIME_D3 );

	myRead1( unit, URE_PLA_SUSPEND_FLAG, URE_MCU_TYPE_PLA, & byte );

	myWrite1( unit, URE_PLA_SUSPEND_FLAG, URE_MCU_TYPE_PLA, byte & ~URE_LINK_CHG_EVENT );

	myRead2( unit, URE_PLA_EXTRA_STATUS, URE_MCU_TYPE_PLA, & value );

	myWrite2( unit, URE_PLA_EXTRA_STATUS, URE_MCU_TYPE_PLA, value & ~URE_LINK_CHANGE_FLAG );

	myRead2( unit, URE_PLA_CONFIG34, URE_MCU_TYPE_PLA, & value );

	myWrite1( unit, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_CONFIG );

	myWrite2( unit, URE_PLA_CONFIG34, URE_MCU_TYPE_PLA, value & ~URE_LINK_OFF_WAKE_EN );

	myWrite1( unit, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_NORAML );

	myRead2( unit, URE_PLA_MAC_PWR_CTRL3, URE_MCU_TYPE_PLA, & value );

	myWrite2( unit, URE_PLA_MAC_PWR_CTRL3, URE_MCU_TYPE_PLA, value & ~URE_PLA_MCU_SPDWN_EN );
}

// --

S32
RTL8153_Init( struct devUnit *unit )
{
	U16 power_cut;
	U16 value;
	U8 tolerance[6];
	S32 rtl8153b;

	rtl8153b = RTL8153B_CHIP( unit );
	power_cut = URE_PWR_EN | URE_PHASE2_EN;

	// Keep USB link power management disabled while the driver owns the NIC.
	if ( rtl8153b )
	{
		UpdateUSBWord( unit, URE_USB_LPM_CONFIG, URE_LPM_U1U2_EN, 0 );

		power_cut |= URE_UPS_EN | URE_USP_PREWAKE;
	}
	else
	{
		memset( tolerance, 0, sizeof( tolerance ) );

		myWrite6( unit, URE_USB_TOLERANCE, URE_MCU_TYPE_USB, tolerance );
	}

	if ( ! WaitAutoload( unit ) )
	{
		return ( FALSE );
	}

	UpdateUSBWord( unit, URE_USB_U2P3_CTRL, URE_U2P3_ENABLE, 0 );

	UpdateUSBWord( unit, URE_USB_POWER_CUT, power_cut, 0 );

	UpdateUSBWord( unit, URE_USB_MISC_0, URE_PCUT_STATUS, 0 );

	RTL8152_Disable_ALDPS( unit );

	myReadReg( unit, URE_OCP_POWER_CFG, & value );

	myWriteReg( unit, URE_OCP_POWER_CFG, value & ~URE_EN_ALDPS );

	if ( ! WaitPHY( unit, TRUE ) )
	{
		return ( FALSE );
	}

	RTL8152_HW_Phy_Cfg( unit );

	if ( ! WaitPHY( unit, FALSE ) )
	{
		return ( FALSE );
	}

	if ( rtl8153b )
	{
		ConfigureRTL8153B( unit );
	}
	else
	{
		ConfigureRTL8153( unit );
	}

	RTL8152_Enable_FC( unit );

	myTallyReset( unit );

	UpdateUSBWord( unit, URE_USB_USB_CTRL, URE_RX_AGG_DISABLE | URE_RX_ZERO_EN, 0 );

	return ( ! unit->unit_USBControlError );
}

// --
