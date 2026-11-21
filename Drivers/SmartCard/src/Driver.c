
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/SmartCard.h"

// --

#define DRIVER_ID USB2SMARTCARD_DRIVER " 1.0 (21.11.2026)\r\n"

struct Library *SysBase;
struct ExecIFace *IExec;

static PTR Driver_Init( PTR dummy, PTR seglist, struct ExecBase *sysbase );

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
Open( struct LibraryManagerInterface *Self, U32 version __attribute__((unused)) )
{
	struct Library *library;

	library = Self->Data.LibBase;
	library->lib_OpenCnt++;
	library->lib_Flags &= ~LIBF_DELEXP;

	return ( library );
}

// --

static PTR
Close( struct LibraryManagerInterface *Self )
{
	Self->Data.LibBase->lib_OpenCnt--;

	return ( NULL );
}

// --

static PTR
Expunge( struct LibraryManagerInterface *Self __attribute__((unused)) )
{
	return ( NULL );
}

// --

static const PTR ManagerVectors[] =
{
	(PTR) Obtain, (PTR) Release, NULL, NULL, (PTR) Open, (PTR) Close,
	(PTR) Expunge, NULL, (PTR) -1
};

static const PTR DriverVectors[] =
{
	(PTR) Obtain, (PTR) Release, NULL, NULL, (PTR) SmartCard_Entry, (PTR) -1
};

static const PTR ClientVectors[] =
{
	(PTR) Obtain, (PTR) Release, NULL, NULL,
	(PTR) SmartCard_GetInfo, (PTR) SmartCard_Command, (PTR) -1
};

static const struct TagItem ManagerTags[] =
{
	{ MIT_Name, (Tag) "__library" },
	{ MIT_VectorTable, (Tag) ManagerVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const struct TagItem DriverTags[] =
{
	{ MIT_Name, (Tag) "main" },
	{ MIT_VectorTable, (Tag) DriverVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const struct TagItem ClientTags[] =
{
	{ MIT_Name, (Tag) USB2SMARTCARD_INTERFACE },
	{ MIT_VectorTable, (Tag) ClientVectors },
	{ MIT_Version, USB2SMARTCARD_VERSION },
	{ TAG_END, 0 }
};

static const PTR Interfaces[] =
{
	(PTR) ManagerTags, (PTR) DriverTags, (PTR) ClientTags, NULL
};

// --

static PTR
Driver_Init( PTR dummy __attribute__((unused)), PTR seglist __attribute__((unused)), struct ExecBase *sysbase )
{
	struct SmartCardDriver *base;

	SysBase = (PTR) sysbase;
	IExec = (PTR) sysbase->MainInterface;

	base = (PTR) IExec->CreateLibraryTags(
		CLT_DataSize, sizeof( *base ),
		CLT_Interfaces, Interfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END );

	if ( ! base )
	{
		return ( NULL );
	}

	base->Library.lib_Node.ln_Type = NT_LIBRARY;
	base->Library.lib_Node.ln_Name = USB2SMARTCARD_DRIVER;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = 1;
	base->Library.lib_IdString = DRIVER_ID;

	IExec->InitSemaphore( & base->Lock );

	IExec->AddLibrary( & base->Library );

	return ( base );
}

// --

static const char VersionTag[] __attribute__((used)) = "\0$VER: " DRIVER_ID;

static const struct Resident DriverResident __attribute__((used)) =
{
	RTC_MATCHWORD, (PTR) & DriverResident, (PTR) ( & DriverResident + 1 ),
	RTF_NATIVE | RTF_COLDSTART, 1, NT_LIBRARY, 0,
	USB2SMARTCARD_DRIVER, DRIVER_ID, (PTR) Driver_Init
};

// --

S32
_start( void )
{
	return ( 0 );
}

// --
