
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

// --

int RTL8152_Control( struct devUnit *unit, U32 command, U16 address, U16 index, U8 *data, U16 length );
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
void RTL8152_SetSpeed( struct devUnit *unit, int AutoNeg, uint16 Speed, int Duplex );
void RTL8152_Enable_FC( struct devUnit *unit );
void RTL8152_Enable_ALDPS( struct devUnit *unit );
void RTL8152_Disable_ALDPS( struct devUnit *unit );
void RTL8152_EEE_Enable( struct devUnit *unit, int Enable );
void RTL8152_Enable_EEE( struct devUnit *unit );
void RTL8152_Init( struct devUnit *unit );
void RTL8152_Exit_OOB( struct devUnit *unit );
void RTL8152_Up( struct devUnit *unit );

#endif
