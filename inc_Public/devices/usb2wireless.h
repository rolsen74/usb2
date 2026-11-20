
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef DEVICES_USB2WIRELESS_H
#define DEVICES_USB2WIRELESS_H

#include <utility/tagitem.h>

// --

// Set with S2_SETOPTIONS for a link authenticated with SAE.
#define USB2WIRELESSINFO_SAE ( TAG_USER + 0x10000 )
// S2_SETKEY packet type for an IGTK in slot 4 or 5, with a six-byte IPN.
#define USB2WIRELESSENC_BIP_CMAC 0x10001

// S2_SETKEY slot for TKIP group key ID 0.
#define USB2WIRELESS_TKIP_GROUP_ZERO 4

// Private SANA-II command to launch the Wi-Fi password requester.
#define USB2WIRELESS_CMD_PROMPT 0xc080

// S2_CONFIGINTERFACE came from the manager itself.
#define USB2WIRELESS_IOF_MANAGER_CONFIG ( 1U << 4 )

#endif
