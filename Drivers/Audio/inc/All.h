/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef DRIVERS_AUDIO_ALL_H
#define DRIVERS_AUDIO_ALL_H

#include <proto/exec.h>
#include <proto/usb2.h>
#include <devices/newstyle.h>
#include <dos/dos.h>

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif
#ifndef USED
#define USED __attribute__((used))
#endif

#define SEC_CODE
#define SEC_RODATA

#define AUDIO_DRIVER_NAME "cls_Audio.Driver"
#define AUDIO_DRIVER_VERSION 1
#define AUDIO_DRIVER_REVISION 5
#define AUDIO_DRIVER_ID AUDIO_DRIVER_NAME " 1.5 (25.11.2026)"

#ifdef USB2_AUDIO_TRACE
#define AUDIO_TRACE( ... ) DebugPrintF( "USB Audio: " __VA_ARGS__ )
#define AUDIO_DRIVER_LABEL AUDIO_DRIVER_ID " [audio trace]\r\n"
#else
#define AUDIO_TRACE( ... ) ((void) 0)
#define AUDIO_DRIVER_LABEL AUDIO_DRIVER_ID "\r\n"
#endif

#include "Audio.h"

#endif
