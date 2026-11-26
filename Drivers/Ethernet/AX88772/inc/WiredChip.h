
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef AX88772_WIRED_CHIP_H
#define AX88772_WIRED_CHIP_H

// --

struct AX88772Rx
{
	U8 Frame[ETHERNET_MTU];
	U8 Header[4];
	U16 Length;
	U16 Used;
	U8 HeaderUsed;
	U8 Padding;
};

#define WIRED_CHIP_FIELDS \
	struct AX88772Rx unit_AXReceive; \
	U16 unit_AXMedium; \
	U8 unit_AXPHY; \
	U8 unit_AXMACValid;

#define WIRED_ADAPTER USB2ETH_ADAPTER_AX88772
#define WIRED_NAME "AX88772"

// Frames can span transfers. The shared RX loop decodes replies in endpoint order.
#define WIRED_RX_REQUEST_COUNT 3
#define WIRED_TX_REQUEST_COUNT 3
#define WIRED_RX_BUFFER_SIZE 2048
#define WIRED_TX_HEADER_SIZE 4
#define WIRED_TX_BUFFER_SIZE ( ETHERNET_MTU + WIRED_TX_HEADER_SIZE + 4 )
#define WIRED_RESET_RX(unit) AX88772_ResetRX( & (unit)->unit_AXReceive )

void AX88772_ResetRX( struct AX88772Rx *rx );

#endif
