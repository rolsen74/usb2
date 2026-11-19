/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

extern const PTR DriverInterfaces[];

// --

SEC_CODE struct AudioDriverBase *Driver_Init( PTR dummy UNUSED, PTR seglist, struct ExecBase *sysbase )
{
struct AudioDriverBase *driver;

	SysBase = (PTR) sysbase;
	IExec = (PTR) sysbase->MainInterface;

	driver = (PTR) CreateLibraryTags(
		CLT_DataSize, sizeof( struct AudioDriverBase ),
		CLT_Interfaces, DriverInterfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END
	);

	if ( ! driver )
	{
		return ( NULL );
	}

	driver->Library.lib_Node.ln_Type = NT_LIBRARY;
	driver->Library.lib_Node.ln_Name = (STR) "Audio.Driver";
	driver->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	driver->Library.lib_Version = 1;
	driver->Library.lib_Revision = 0;
	driver->Library.lib_IdString = (STR) AUDIO_DRIVER_LABEL;
	driver->SegList = seglist;
	InitSemaphore( & driver->Context.Lock );

	AddLibrary( (PTR) driver );

	AUDIO_TRACE( "Audio.Driver loaded\n" );

	return ( driver );
}

// --
