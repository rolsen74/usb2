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

SEC_CODE struct SerialDriverBase *Driver_Init( PTR dummy UNUSED, PTR seglist, struct ExecBase *sysbase )
{
struct SerialDriverBase *driver;

	SysBase = (PTR) sysbase;
	IExec = (PTR) sysbase->MainInterface;

	driver = (PTR) CreateLibraryTags(
		CLT_DataSize, sizeof( struct SerialDriverBase ),
		CLT_Interfaces, DriverInterfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END
	);

	if ( ! driver )
	{
		return ( NULL );
	}

	driver->Library.lib_Node.ln_Type = NT_LIBRARY;
	driver->Library.lib_Node.ln_Name = (STR) SERIAL_DRIVER_NAME;
	driver->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	driver->Library.lib_Version = SERIAL_DRIVER_VERSION;
	driver->Library.lib_Revision = SERIAL_DRIVER_REVISION;
	driver->Library.lib_IdString = (STR) SERIAL_DRIVER_ID;
	driver->SegList = seglist;
	driver->Context.IExec = IExec;

	DOSBase = OpenLibrary( "dos.library", 50 );

	if ( DOSBase )
	{
		IDOS = (PTR) GetInterface( DOSBase, "main", 1, NULL );
	}

	if ( ! IDOS )
	{
		if ( DOSBase )
		{
			CloseLibrary( DOSBase );
			DOSBase = NULL;
		}

		DeleteLibrary( (PTR) driver );

		return ( NULL );
	}

	driver->DOSBase = DOSBase;
	driver->Context.IDOS = IDOS;

	AddLibrary( (PTR) driver );

	return ( driver );
}

// --
