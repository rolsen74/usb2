
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef WIFI_EMBEDDED_OUTPUT_H
#define WIFI_EMBEDDED_OUTPUT_H

void WiFi_Trace( const char *format, ... ) __attribute__((format(printf, 1, 2)));

// Route the shared manager's output away from the Shell-only stdio streams.
#define printf( ... ) WiFi_Trace( __VA_ARGS__ )
#define fprintf( stream, ... ) WiFi_Trace( __VA_ARGS__ )
#define fputc( character, stream ) WiFi_Trace( "%c", (int) ( character ) )
#define fputs( message, stream ) WiFi_Trace( "%s", ( message ) )
#define puts( message ) WiFi_Trace( "%s\n", ( message ) )
#define fflush( stream ) ((void) 0)

#endif
