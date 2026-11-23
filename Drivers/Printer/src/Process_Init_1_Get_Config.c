
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include <stdio.h>

// --

#define CFGBUF_SIZE 4096

static void
__myProcess_1_Get_Config( struct PrinterUnit *unit )
{
	struct RDArgs *args;
	struct RDArgs *source;
	struct ConfigOptions options;

	char filename[64];
	STR buffer;
	S32 length;

	if ( unit->unit_Dot4_Active )
	{
		return;
	}

	unit->unit_Buffer_Size = 16 * 1024;

	buffer = AllocVecTags( CFGBUF_SIZE, TAG_END );

	if ( ! buffer )
	{
		return;
	}

	snprintf( filename, sizeof( filename ), "usb2_printer.device/unit%lu", unit->unit_UnitNr );

	length = GetVar( filename, buffer, CFGBUF_SIZE - 2, 0 );

	if ( ( length <= 0 ) || ( length >= CFGBUF_SIZE - 2 ) )
	{
		goto bailout;
	}

	buffer[length++] = '\n';
	buffer[length] = 0;

	source = AllocDosObjectTagList( DOS_RDARGS, NULL );

	if ( ! source )
	{
		goto bailout;
	}

	memset( & options, 0, sizeof( options ) );

	source->RDA_Source.CS_Buffer = buffer;
	source->RDA_Source.CS_Length = length;
	source->RDA_Flags = RDAF_NOPROMPT;

	args = ReadArgs( CONFIGTEMPLATE, (PTR) & options, source );

	if ( args )
	{
		if ( options.BufferSize )
		{
			unit->unit_Buffer_Size = MIN( *options.BufferSize, 1024 ) * 1024;
		}

		FreeArgs( args );
	}
	else
	{
		DebugPrintF( "USB2 printer: invalid settings in %s\n", filename );
	}

	FreeDosObject( DOS_RDARGS, source );

bailout:

	FreeVec( buffer );
}

// --
