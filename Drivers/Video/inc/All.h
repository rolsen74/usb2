/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef DRIVERS_VIDEO_ALL_H
#define DRIVERS_VIDEO_ALL_H

#include <proto/exec.h>
#include <proto/usb2.h>
#include <devices/newstyle.h>
#include <devices/timer.h>
#include <interfaces/timer.h>
#include <dos/dos.h>

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif
#ifndef USED
#define USED __attribute__((used))
#endif

#define SEC_CODE
#define SEC_RODATA

#define VIDEO_DRIVER_NAME "cls_Video.Driver"
#define VIDEO_DRIVER_VERSION 1
#define VIDEO_DRIVER_REVISION 3
#define VIDEO_DRIVER_ID VIDEO_DRIVER_NAME " 1.3 (24.11.2026)\r\n"

#include "Video.h"

#endif
