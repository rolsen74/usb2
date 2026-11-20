
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2RTL8188_H
#define USB2RTL8188_H

// Only valid on a cooked CMD_WRITE for an initial WPA2 EAPOL-Key message 4.
#define USB2RTL8188IOF_CLEAR_EAPOL ( 1U << 4 )

#include <devices/usb2wireless.h>

// S2_SETKEY slot for TKIP group key ID 0, separate from the pairwise key.
#define USB2RTL8188_TKIP_GROUP_ZERO USB2WIRELESS_TKIP_GROUP_ZERO

#endif
