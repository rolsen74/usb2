
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

#include <stdarg.h>
#include <stdio.h>

#include "EmbeddedOutput.h"

// --

void
WiFi_Trace( const char *format, ... )
{
	va_list args;
	char buffer[512];

	va_start( args, format );

	vsnprintf( buffer, sizeof( buffer ), format, args );

	va_end( args );

	DebugPrintF( "%s", buffer );
}

// --
