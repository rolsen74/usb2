
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

static S32
WaitLinkList( struct devUnit *unit )
{
	U32 count;
	U8 status;

	for( count = 0; count < 50; count++ )
	{
		if ( myRead1( unit, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA, & status ) )
		{
			return ( FALSE );
		}

		if ( status & URE_LINK_LIST_READY )
		{
			return ( TRUE );
		}

		IDOS->Delay( 1 );
	}

	IExec->DebugPrintF( "RTL8152: timed out waiting for the OOB link list\n" );

	return ( FALSE );
}

// --

static S32
ResetGigabitEndpoints( struct devUnit *unit )
{
	U8 reset;
	U8 endpoints;

	endpoints = URE_BMU_RESET_EP_IN | URE_BMU_RESET_EP_OUT;

	if ( ( ! RTL8152_Nic_Reset( unit ) ) ||
		( myRead1( unit, URE_USB_BMU_RESET, URE_MCU_TYPE_USB, & reset ) ) ||
		( myWrite1( unit, URE_USB_BMU_RESET, URE_MCU_TYPE_USB, reset & ~endpoints ) ) ||
		( myWrite1( unit, URE_USB_BMU_RESET, URE_MCU_TYPE_USB, reset | endpoints ) ) )
	{
		return ( FALSE );
	}

	return ( TRUE );
}

// --

static void
ConfigureGigabitFIFO( struct devUnit *unit, U32 speed )
{
	U32 early_size;
	U32 timeout;

	// Leave room in the transfer for the next frame and its aligned descriptor.
	early_size = RTL8152_RX_BUFFER_SIZE - RTL8152_RMS - RTL8152_RX_DESCRIPTOR_SIZE - 8;

	myWrite4( unit, URE_PLA_RXFIFO_CTRL0, URE_MCU_TYPE_PLA, URE_RXFIFO_THR1_NORMAL );

	myWrite2( unit, URE_PLA_RXFIFO_CTRL1, URE_MCU_TYPE_PLA, URE_RXFIFO_THR2_NORMAL );

	myWrite2( unit, URE_PLA_RXFIFO_CTRL2, URE_MCU_TYPE_PLA, URE_RXFIFO_THR3_NORMAL );

	myWrite4( unit, URE_PLA_TXFIFO_CTRL, URE_MCU_TYPE_PLA, URE_TXFIFO_THR_NORMAL2 );

	if ( RTL8153B_CHIP( unit ) )
	{
		myWrite4( unit, URE_USB_RX_BUF_TH, URE_MCU_TYPE_USB, URE_RX_THR_B );

		myWrite2( unit, URE_USB_RX_EARLY_AGG, URE_MCU_TYPE_USB, 158 );

		myWrite2( unit, URE_USB_RX_EXTRA_AGG_TMR, URE_MCU_TYPE_USB, 1875 );

		myWrite2( unit, URE_USB_RX_EARLY_SIZE, URE_MCU_TYPE_USB, early_size / 8 );

		myWrite1( unit, URE_USB_UPT_RXDMA_OWN, URE_MCU_TYPE_USB, URE_OWN_UPDATE | URE_OWN_CLEAR );
	}
	else
	{
		timeout = URE_COALESCE_SLOW;

		if ( speed == USBSPEED_High )
		{
			timeout = URE_COALESCE_HIGH;
		}
		else if ( speed >= USBSPEED_Super )
		{
			timeout = URE_COALESCE_SUPER;
		}

		myWrite2( unit, URE_USB_RX_EARLY_AGG, URE_MCU_TYPE_USB, timeout / 8 );

		myWrite2( unit, URE_USB_RX_EARLY_SIZE, URE_MCU_TYPE_USB, early_size / 4 );
	}
}

// --

S32
RTL8152_Exit_OOB( struct devUnit *unit )
{
U32 val4;
U32 speed;
U32 threshold2;
U32 threshold3;
uint16 val2;
uint8 val1;

	myRead4(  unit, URE_PLA_RCR, URE_MCU_TYPE_PLA, & val4 );
	myWrite4( unit, URE_PLA_RCR, URE_MCU_TYPE_PLA, val4 & ~URE_RCR_ACPT_ALL );

	rxdy_gated_en( unit, TRUE );

	RTL8153_Teredo_Off( unit );

	if ( unit->unit_Support_GMII )
	{
		if ( ! ResetGigabitEndpoints( unit ) )
		{
			return ( FALSE );
		}
	}
	else
	{
		RTL8152_HW_Phy_Cfg( unit );
	}

	myWrite1( unit, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_NORAML );
	myWrite1( unit, URE_PLA_CR, URE_MCU_TYPE_PLA, 0x00 );

	myRead1(  unit, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA, & val1 );
	myWrite1( unit, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA, val1 & ~URE_NOW_IS_OOB );

	myRead2(  unit, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA, & val2 );
	myWrite2( unit, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA, val2 & ~URE_MCU_BORW_EN );

	if ( ! WaitLinkList( unit ) )
	{
		return ( FALSE );
	}

	myRead2( unit, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA, & val2 );

	myWrite2( unit, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA, val2 | URE_RE_INIT_LL );

	if ( ( ! WaitLinkList( unit ) ) ||
		( ( ! unit->unit_Support_GMII ) && ( ! RTL8152_Nic_Reset( unit ) ) ) )
	{
		return ( FALSE );
	}

	if ( unit->unit_IUSB2->USB2_Attr_Get( unit->unit_USBRegister->FKT_ID,
		USB2Tag_Attr_Speed, & speed, sizeof( speed ) ) != sizeof( speed ) )
	{
		return ( FALSE );
	}

	if ( unit->unit_Support_GMII )
	{
		RTL_RX_VLan_En( unit, FALSE );

		myWrite2( unit, URE_PLA_RMS, URE_MCU_TYPE_PLA, RTL8152_RMS );

		myWrite1( unit, URE_PLA_MTPS, URE_MCU_TYPE_PLA, URE_MTPS_JUMBO );

		myRead2( unit, URE_PLA_TCR0, URE_MCU_TYPE_PLA, & val2 );

		myWrite2( unit, URE_PLA_TCR0, URE_MCU_TYPE_PLA, val2 | URE_TCR0_AUTO_FIFO );

		if ( ! RTL8152_Nic_Reset( unit ) )
		{
			return ( FALSE );
		}

		ConfigureGigabitFIFO( unit, speed );

		return ( ! unit->unit_USBControlError );
	}

	threshold2 = ( speed == USBSPEED_Full ) ? URE_RXFIFO_THR2_FULL : URE_RXFIFO_THR2_HIGH;
	threshold3 = ( speed == USBSPEED_Full ) ? URE_RXFIFO_THR3_FULL : URE_RXFIFO_THR3_HIGH;

	myWrite4( unit, URE_PLA_RXFIFO_CTRL0, URE_MCU_TYPE_PLA, URE_RXFIFO_THR1_NORMAL );

	myWrite4( unit, URE_PLA_RXFIFO_CTRL1, URE_MCU_TYPE_PLA, threshold2 );

	myWrite4( unit, URE_PLA_RXFIFO_CTRL2, URE_MCU_TYPE_PLA, threshold3 );

	/* TX share fifo free credit full threshold */
	myWrite4( unit, URE_PLA_TXFIFO_CTRL, URE_MCU_TYPE_PLA, URE_TXFIFO_THR_NORMAL );

	myWrite1( unit, URE_USB_TX_AGG,		URE_MCU_TYPE_USB,	URE_TX_AGG_MAX_THRESHOLD );
	myWrite4( unit, URE_USB_RX_BUF_TH,	URE_MCU_TYPE_USB,	URE_RX_THR_HIGH );
	myWrite4( unit, URE_USB_TX_DMA,		URE_MCU_TYPE_USB,	URE_TEST_MODE_DISABLE | URE_TX_SIZE_ADJUST1 );

	RTL_RX_VLan_En( unit, FALSE );

	myWrite2( unit, URE_PLA_RMS, URE_MCU_TYPE_PLA, RTL8152_RMS );

	myRead2(  unit, URE_PLA_TCR0, URE_MCU_TYPE_PLA, & val2 );
	myWrite2( unit, URE_PLA_TCR0, URE_MCU_TYPE_PLA, val2 | URE_TCR0_AUTO_FIFO );

	return ( ! unit->unit_USBControlError );
}

// --
