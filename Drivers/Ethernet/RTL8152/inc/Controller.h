
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8152_CONTROLLER_H
#define RTL8152_CONTROLLER_H

// --

#include "RTL8152.h"

// --

#ifndef LE_SWAP32
#define LE_SWAP32(x)	( (( x & 0x000000ff ) << 24 ) \
						| (( x & 0x0000ff00 ) << 8  ) \
						| (( x & 0x00ff0000 ) >> 8  ) \
						| (( x & 0xff000000 ) >> 24 ) )
#endif

#define RTL8152_RMS		( 1518 + 18 )	// ( VLAN_ETH_FRAME_LEN + VLAN_HLEN )

#define URE_PLA_RSTTALLY		0xe800

// PLA_RSTTALLY
#define URE_TALLY_RESET			0x0001

// RTL8153B
#define URE_CHIP_VER_6000		0x40
#define URE_CHIP_VER_6010		0x80
#define RTL8153B_CHIP(unit)		( (unit)->unit_ChipVersion & ( URE_CHIP_VER_6000 | URE_CHIP_VER_6010 ) )

#define URE_PLA_SUSPEND_FLAG		0xd38a
#define URE_PLA_INDICATE_FLAG		0xd38c
#define URE_PLA_EXTRA_STATUS		0xd398
#define URE_PLA_CONFIG34		0xe820
#define URE_LINK_CHG_EVENT		0x01
#define URE_UPCOMING_RUNTIME_D3		0x01
#define URE_LINK_CHANGE_FLAG		0x0100
#define URE_LINK_OFF_WAKE_EN		0x0008
#define URE_USB_MSC_TIMER		0xcbfc
#define URE_USB_LPM_CONFIG		0xcfd8
#define URE_USB_RX_EARLY_SIZE		0xd42e
#define URE_USB_RX_EXTRA_AGG_TMR		0xd432
#define URE_USB_UPT_RXDMA_OWN		0xd437
#define URE_USB_BMU_RESET		0xd4b0
#define URE_USB_U1U2_TIMER		0xd4da
#define URE_LPM_U1U2_EN			0x0001
#define URE_UPS_EN			0x0010
#define URE_USP_PREWAKE			0x0020
#define URE_BMU_RESET_EP_IN		0x01
#define URE_BMU_RESET_EP_OUT		0x02
#define URE_RX_THR_B			0x00010001
#define URE_OWN_UPDATE			0x01
#define URE_OWN_CLEAR			0x02
#define URE_RX_VLAN_8153B		0x00c0
#define URE_PLA_MCU_SPDWN_EN		0x4000
#define URE_PHY_STAT_EXT_INIT		2
#define URE_MTPS_JUMBO			192

// OCP_EEE_CONFIG1
#define EEE_10_CAP				0x2000
#define EEE_NWAY_EN				0x1000
#define TX_QUIET_EN				0x0200
#define RX_QUIET_EN				0x0100
#define sd_rise_time_mask		0x0070
#define sd_rise_time(x)			( x << 4 )

// OCP_EEE_CONFIG2
#define RG_DACQUIET_EN			0x0400
#define RG_LDVQUIET_EN			0x0200

// OCP_EEE_CONFIG3
#define fast_snr_mask			0xff80
#define fast_snr(x)				( x << 7 )

// OCP_EEE_AR
#define FUN_ADDR				0x0000
#define FUN_DATA				0x4000

// MDIO
#define MDIO_MMD_AN				7
#define MDIO_AN_EEE_ADV			60
#define MDIO_EEE_100TX			0x0002

// MII
#define MII_BMCR				0x00
#define MII_ADVERTISE			0x04
#define MII_CTRL1000			0x09
#define ADVERTISE_10HALF		0x0020
#define ADVERTISE_10FULL		0x0040
#define ADVERTISE_100HALF		0x0080
#define ADVERTISE_100FULL		0x0100
#define ADVERTISE_PAUSE_CAP		0x0400
#define ADVERTISE_PAUSE_ASYM	0x0800

#define ADVERTISE_1000HALF		0x0100
#define ADVERTISE_1000FULL		0x0200

// BMCR
#define BMCR_SPEED1000			0x0040
#define BMCR_FULLDPLX			0x0100
#define BMCR_ANRESTART			0x0200
#define BMCR_PDOWN				0x0800
#define BMCR_ANENABLE			0x1000
#define BMCR_SPEED100			0x2000
#define BMCR_RESET				0x8000

enum
{
	SPEED_10,
	SPEED_100,
	SPEED_1000,
};

S32 RTL8153_Init( struct devUnit *unit );

// --

int RTL8152_Control( struct devUnit *unit, U32 command, U16 address, U16 index, U8 *data, U16 length );
void RTL8152_ReportUSBConfiguration( struct devUnit *unit );
int myReadMem( struct devUnit *unit, U16 address, U16 index, U8 *data, U16 length );
int myRead1( struct devUnit *unit, uint16 reg, uint16 index, uint8 *result );
int myRead2( struct devUnit *unit, uint16 reg, uint16 index, uint16 *result );
U32 myRead4( struct devUnit *unit, uint16 reg, uint16 index, U32 *result );
int myReadReg( struct devUnit *unit, uint16 addr, uint16 *retval );
int myReadMDIO( struct devUnit *unit, uint16 reg_addr, uint16 *retval );
int myWriteMem( struct devUnit *unit, U16 address, U16 index, U8 *data, U16 length );
int myWrite1( struct devUnit *unit, U16 reg, U16 index, U32 value );
int myWrite2( struct devUnit *unit, U16 reg, U16 index, U32 value );
int myWrite4( struct devUnit *unit, uint16 reg, uint16 index, U32 val );
int myWrite6( struct devUnit *unit, U16 address, U16 index, const U8 *source );
int myWriteReg( struct devUnit *unit, uint16 addr, uint16 data );
int myWriteMDIO( struct devUnit *unit, uint16 reg_addr, uint16 data );
int myWriteMMD( struct devUnit *unit, uint16 dev, uint16 reg, uint16 data );
int myReadChipID( struct devUnit *unit );
void rxdy_gated_en( struct devUnit *unit, int Enable );
void myTallyReset( struct devUnit *unit );
void RTL_RX_VLan_En( struct devUnit *unit, int Enable );
void RTL8152_HW_Phy_Cfg( struct devUnit *unit );
void RTL8153_Teredo_Off( struct devUnit *unit );
int RTL8152_Nic_Reset( struct devUnit *unit );
int RTL8152_GetMacAddress( struct devUnit *unit );
void RTL8152_Power_Cut( struct devUnit *unit, int Enable );
int RTL8152_SetSpeed( struct devUnit *unit, int AutoNeg, uint16 Speed, int Duplex );
void RTL8152_Enable_FC( struct devUnit *unit );
void RTL8152_Enable_ALDPS( struct devUnit *unit );
void RTL8152_Disable_ALDPS( struct devUnit *unit );
void RTL8152_EEE_Enable( struct devUnit *unit, int Enable );
void RTL8152_Enable_EEE( struct devUnit *unit );
void RTL8152_Init( struct devUnit *unit );
S32 RTL8152_Exit_OOB( struct devUnit *unit );
S32 RTL8152_Up( struct devUnit *unit );

#endif
