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

SEC_CODE struct VideoDriverBase *Driver_Init( PTR dummy UNUSED, PTR seglist, struct ExecBase *sysbase )
{
struct VideoDriverBase *driver;

	SysBase = (PTR) sysbase;
	IExec = (PTR) sysbase->MainInterface;

	driver = (PTR) CreateLibraryTags(
		CLT_DataSize, sizeof( struct VideoDriverBase ),
		CLT_Interfaces, DriverInterfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END
	);

	if ( ! driver )
	{
		return ( NULL );
	}

	driver->Library.lib_Node.ln_Type = NT_LIBRARY;
	driver->Library.lib_Node.ln_Name = (STR) VIDEO_DRIVER_NAME;
	driver->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	driver->Library.lib_Version = VIDEO_DRIVER_VERSION;
	driver->Library.lib_Revision = VIDEO_DRIVER_REVISION;
	driver->Library.lib_IdString = (STR) VIDEO_DRIVER_ID;
	driver->SegList = seglist;
	InitSemaphore( & driver->Context.Lock );

	AddLibrary( (PTR) driver );

	return ( driver );
}

// --
