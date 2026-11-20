
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include "SDRTest.h"

// --

S32
SDR_SaveIQ( const char *path, const U8 *samples, U32 bytes )
{
	BPTR file;
	S32 written;
	S32 closed;

	file = Open( path, MODE_NEWFILE );

	if ( ! file )
	{
		PrintFault( IoErr(), path );

		return ( FALSE );
	}

	written = Write( file, samples, bytes );

	closed = Close( file );

	if ( ( written != (S32) bytes ) || ( ! closed ) )
	{
		Printf( "Could not save the complete I/Q capture to %s.\n", path );

		return ( FALSE );
	}

	Printf( "Saved %lu bytes of unsigned 8-bit I/Q to %s.\n", (unsigned long) bytes, path );

	return ( TRUE );
}

// --
