
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef PEGASUS_CHIP_H
#define PEGASUS_CHIP_H

// --

#define PEG_READ_REGS         0xf0
#define PEG_WRITE_REGS        0xf1

#define PEG_ETH_CTRL0         0x00
#define PEG_ETH_CTRL1         0x01
#define PEG_ETH_ID            0x10
#define PEG_RX_OPTIONS        0x1d
#define PEG_EEPROM_OFFSET     0x20
#define PEG_EEPROM_DATA       0x21
#define PEG_EEPROM_CTRL       0x23
#define PEG_PHY_ADDR          0x25
#define PEG_PHY_DATA          0x26
#define PEG_PHY_CTRL          0x28
#define PEG_TX_STATUS         0x2b
#define PEG_WAKE_CTRL         0x78
#define PEG_PHY_SELECT        0x7b
#define PEG_GPIO0             0x7e
#define PEG_GPIO1             0x7f

#define PEG_RX_APPEND_STATUS  0x08
#define PEG_RX_ALL_MULTICAST  0x02
#define PEG_RX_ENABLE         0x40
#define PEG_TX_ENABLE         0x80
#define PEG_MAC_RESET         0x08
#define PEG_MAC_SPEED100      0x10
#define PEG_MAC_FULL_DUPLEX   0x20
#define PEG_EP3_CLEAR_STATUS  0x01

#define PEG_EEPROM_READ       0x02
#define PEG_EEPROM_DONE       0x04
#define PEG_PHY_READ          0x40
#define PEG_PHY_WRITE         0x20
#define PEG_PHY_DONE          0x80

#define PEG_MII_CONTROL       0
#define PEG_MII_STATUS        1
#define PEG_MII_ADVERTISE     4
#define PEG_MII_PARTNER       5

#define PEG_MII_AUTONEG       0x1000
#define PEG_MII_SPEED100      0x2000
#define PEG_MII_FULL_DUPLEX   0x0100
#define PEG_MII_LINK          0x0004
#define PEG_MII_NEGOTIATED    0x0020
#define PEG_MII_MEDIA         0x7808
#define PEG_MII_100_FULL      0x0100
#define PEG_MII_100_HALF      0x0080
#define PEG_MII_10_FULL       0x0040
#define PEG_MII_10_HALF       0x0020

enum PegasusMACState
{
	PEG_MAC_STOPPED,
	PEG_MAC_UNVERIFIED,
	PEG_MAC_READY
};

S32 Pegasus_Read( struct devUnit *unit, U8 reg, U8 *data, U16 length );
S32 Pegasus_Write( struct devUnit *unit, U8 reg, U8 *data, U16 length );
S32 Pegasus_WriteByte( struct devUnit *unit, U8 reg, U8 value );
S32 Pegasus_WaitRegister( struct devUnit *unit, U8 reg, U8 mask, U8 expected );
S32 Pegasus_ReadPHY( struct devUnit *unit, U8 phy, U8 reg, U16 *value );
S32 Pegasus_WritePHY( struct devUnit *unit, U8 phy, U8 reg, U16 value );
void Pegasus_TraceMACStatus( struct devUnit *unit );

#endif
