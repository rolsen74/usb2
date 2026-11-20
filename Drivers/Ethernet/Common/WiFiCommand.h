
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_WIFI_COMMAND_H
#define USB2_WIFI_COMMAND_H

#include <devices/sana2.h>
#include <devices/usb2wireless.h>
#include "Types.h"

void Wireless_StartAuto( U32 adapter, U32 unit );
void Wireless_StartPassword( struct IOSana2Req *request );

#endif
