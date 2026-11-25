
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef AX88772_CHIP_H
#define AX88772_CHIP_H

// --

#define AX_SW_MII          0x06
#define AX_READ_MII        0x07
#define AX_WRITE_MII       0x08
#define AX_MII_STATUS      0x09
#define AX_HW_MII          0x0a
#define AX_READ_EEPROM     0x0b
#define AX_RX_CONTROL      0x10
#define AX_WRITE_IPG       0x12
#define AX_READ_MAC        0x13
#define AX_WRITE_MAC       0x14
#define AX_READ_PHY        0x19
#define AX_WRITE_MEDIUM    0x1b
#define AX_WRITE_GPIO      0x1f
#define AX_RESET           0x20
#define AX_SELECT_PHY      0x22
#define AX_BULK_QUEUE      0x2a

#define AX_CHIP_MASK       0x70
#define AX_CHIP_772        0x00
#define AX_CHIP_772A       0x10
#define AX_CHIP_772B       0x20
#define AX_HOST_ENABLE     0x01
#define AX_PHY_INTERNAL    0x10
#define AX_PHY_SERIAL      0x10
#define AX_GPIO_RELOAD     0x80
#define AX_GPIO_PHY_POWER  0x30
#define AX_RESET_EXT_PHY   0x08
#define AX_RESET_INT_PHY   0x20
#define AX_POWER_DOWN_PHY  0x40

#define AX_RX_ON           0x0080
#define AX_RX_BROADCAST    0x0008
#define AX_RX_ALL_MULTI    0x0002

#define AX_MEDIUM_RX       0x0100
#define AX_MEDIUM_SPEED100 0x0200
#define AX_MEDIUM_DUPLEX   0x0002
#define AX_MEDIUM_AC       0x0004
#define AX_MEDIUM_RX_PAUSE 0x0010
#define AX_MEDIUM_TX_PAUSE 0x0020

#define MII_CONTROL        0
#define MII_STATUS         1
#define MII_PHY_ID1        2
#define MII_PHY_ID2        3
#define MII_ADVERTISE      4
#define MII_PARTNER        5

#define MII_AUTONEG        0x1000
#define MII_RESTART        0x0200
#define MII_SPEED100       0x2000
#define MII_DUPLEX         0x0100
#define MII_LINK_UP        0x0004
#define MII_NEGOTIATED     0x0020

#define MII_ADV_SELECTOR   0x0001
#define MII_ADV_10_HALF    0x0020
#define MII_ADV_10_FULL    0x0040
#define MII_ADV_100_HALF   0x0080
#define MII_ADV_100_FULL   0x0100
#define MII_ADV_PAUSE      0x0400

S32 AX88772_Read( struct devUnit *unit, U8 command, U16 value, U16 index, U8 *data, U16 size );
S32 AX88772_Write( struct devUnit *unit, U8 command, U16 value, U16 index );
S32 AX88772_StartMDIO( struct devUnit *unit );
S32 AX88772_EndMDIO( struct devUnit *unit );
S32 AX88772_ReadPHY( struct devUnit *unit, U16 reg, U16 *value );
S32 AX88772_WritePHY( struct devUnit *unit, U16 reg, U16 value );

#endif
