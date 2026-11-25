
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_WIRELESS_SANA2_H
#define USB2_WIRELESS_SANA2_H

#include <utility/tagitem.h>
#include "Sana2Commands.h"

// --

#define S2_GETSIGNALQUALITY 0xc010
#define S2_GETNETWORKS 0xc011
#define S2_SETOPTIONS 0xc012
#define S2_SETKEY 0xc013
#define S2_GETNETWORKINFO 0xc014
#define S2_GETRADIOBANDS 0xc017
#define S2_GETCRYPTTYPES 0xc018

#define S2ENC_NONE 0
#define S2ENC_TKIP 2
#define S2ENC_CCMP 3

#define S2INFO_SSID ( TAG_USER + 0 )
#define S2INFO_BSSID ( TAG_USER + 1 )
#define S2INFO_AuthTypes ( TAG_USER + 2 )
#define S2INFO_AssocID ( TAG_USER + 3 )
#define S2INFO_Encryption ( TAG_USER + 4 )
#define S2INFO_PortType ( TAG_USER + 5 )
#define S2INFO_BeaconInterval ( TAG_USER + 6 )
#define S2INFO_Channel ( TAG_USER + 7 )
#define S2INFO_WPAInfo ( TAG_USER + 12 )
#define S2INFO_Band ( TAG_USER + 13 )
#define S2INFO_DefaultKeyNo ( TAG_USER + 14 )
#define S2INFO_OperState ( TAG_USER + 20 )

#define S2PORT_MANAGED 7

#endif
