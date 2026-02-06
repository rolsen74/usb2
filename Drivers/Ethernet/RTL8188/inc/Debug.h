
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_DEBUG_H
#define INC_DEBUG_H

// --

#include "All.h"

// --

#if defined( DO_ERROR ) || defined( DO_INFO ) || defined( DO_DEBUG )

void VARARGS68K _USBDEBUG( U32 lvl, STR fmt, ... );

#ifdef DO_ERROR
#define USBERROR( fmt, ... )	_USBDEBUG( 2, fmt, ##__VA_ARGS__ )
#else
#define USBERROR( fmt, ... )	((void)0)
#endif

#ifdef DO_INFO
#define USBINFO( fmt, ... )		_USBDEBUG( 1, fmt, ##__VA_ARGS__ )
#else
#define USBINFO( fmt, ... )		((void)0)
#endif

#ifdef DO_DEBUG
#define USBDEBUG( fmt, ... )	_USBDEBUG( 0, fmt, ##__VA_ARGS__ )
#else
#define USBDEBUG( fmt, ... )	((void)0)
#endif

#else

 #define USBERROR( fmt, ... )	((void)0)
 #define USBINFO(  fmt, ... )	((void)0)
 #define USBDEBUG( fmt, ... )	((void)0)

#endif

// --

#endif
