
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8152_ALL_H
#define RTL8152_ALL_H

// --

#define __S2QUAD_DEFINED
typedef unsigned long long S2QUAD;

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/usb2.h>
#include <proto/utility.h>

#include <devices/newstyle.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <exec/emulation.h>
#include <exec/execbase.h>
#include <interfaces/timer.h>

#include <stddef.h>
#include <string.h>

// --

#include "Types.h"
#include "Debug.h"
#include "Device.h"
#include "Unit.h"

#endif
