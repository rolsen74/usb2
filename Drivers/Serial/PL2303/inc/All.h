/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef DRIVERS_SERIAL_PL2303_ALL_H
#define DRIVERS_SERIAL_PL2303_ALL_H

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/usb2.h>

#include <devices/newstyle.h>

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#ifndef USED
#define USED __attribute__((used))
#endif

#define SEC_CODE
#define SEC_RODATA

#include "Serial.h"

#endif
