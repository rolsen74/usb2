
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8152_WIRED_CHIP_H
#define RTL8152_WIRED_CHIP_H

#define WIRED_CHIP_FIELDS uint16 unit_ChipVersion;
#define WIRED_ADAPTER USB2ETH_ADAPTER_RTL8152
#define WIRED_NAME "RTL8152"
#define WIRED_RX_REQUEST_COUNT 3
#define WIRED_TX_REQUEST_COUNT 3
#define WIRED_RX_BUFFER_SIZE 16384
#define WIRED_TX_HEADER_SIZE 8
#define WIRED_TX_BUFFER_SIZE ( ETHERNET_MTU + WIRED_TX_HEADER_SIZE )

#define RTL8152_RX_DESCRIPTOR_SIZE 24
#define RTL8152_RX_LENGTH_MASK 0x7fff
#define RTL8152_TX_SEGMENTS 0xc0000000U

#define WIRED_RESET_RX(unit) ((void) (unit))

#endif
