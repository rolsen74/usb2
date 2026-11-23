
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static U32
Obtain( struct Interface *Self )
{
	return ( ++Self->Data.RefCount );
}

// --

static U32
Release( struct Interface *Self )
{
	return ( --Self->Data.RefCount );
}

// --

static PTR
Driver_Open( struct LibraryManagerInterface *Self, U32 version )
{
	struct Library *base;

	base = Self->Data.LibBase;

	if ( version > base->lib_Version )
	{
		return ( NULL );
	}

	base->lib_OpenCnt++;

	return ( base );
}

// --

static PTR
Driver_Close( struct LibraryManagerInterface *Self )
{
	Self->Data.LibBase->lib_OpenCnt--;

	return ( NULL );
}

// --

static PTR
Driver_Expunge( struct LibraryManagerInterface *Self UNUSED )
{
	// Keep the USB class driver resident across hotplug.
	return ( NULL );
}

// --

static const PTR ManagerVectors[] =
{
	(PTR) Obtain, (PTR) Release, NULL, NULL,
	(PTR) Driver_Open, (PTR) Driver_Close, (PTR) Driver_Expunge, NULL, (PTR) -1
};

static const struct TagItem ManagerTags[] =
{
	{ MIT_Name, (Tag) "__library" },
	{ MIT_VectorTable, (Tag) ManagerVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const PTR DriverVectors[] =
{
	(PTR) Obtain, (PTR) Release, NULL, NULL, (PTR) Printer_Entry, (PTR) -1
};

static const struct TagItem DriverTags[] =
{
	{ MIT_Name, (Tag) "main" },
	{ MIT_VectorTable, (Tag) DriverVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const PTR Interfaces[] = { (PTR) ManagerTags, (PTR) DriverTags, NULL };

// --

static PTR
Driver_Init( PTR dummy UNUSED, PTR seglist UNUSED, struct ExecBase *sysbase )
{
	struct Library *base;

	SysBase = (PTR) sysbase;
	IExec = (PTR) sysbase->MainInterface;

	base = CreateLibraryTags(
		CLT_DataSize, sizeof( *base ),
		CLT_Interfaces, Interfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END );

	if ( ! base )
	{
		return ( NULL );
	}

	base->lib_Node.ln_Type = NT_LIBRARY;
	base->lib_Node.ln_Name = DEVNAME;
	base->lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->lib_Version = VERSION;
	base->lib_Revision = REVISION;
	base->lib_IdString = VSTRING;

	if ( ! Resources_Init() )
	{
		Resources_Free();

		DeleteLibrary( base );

		return ( NULL );
	}

	AddLibrary( base );

	return ( base );
}

// --

static USED const char verstag[] = VERSTAG;

static USED const struct Resident driver_res =
{
	RTC_MATCHWORD,
	(PTR) & driver_res,
	(PTR) ( & driver_res + 1 ),
	RTF_NATIVE | RTF_COLDSTART,
	VERSION,
	NT_LIBRARY,
	0,
	(STR) DEVNAME,
	(STR) VSTRING,
	(PTR) Driver_Init
};

// --

S32
_start( STR argstring UNUSED, S32 arglen UNUSED, struct ExecBase *sysbase UNUSED )
{
	return ( 0 );
}

// --
