
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#include "LibraryConfig.h"

struct WPALibraryBase
{
	struct Library Library;
	BPTR SegList;
};

struct ExecIFace *IExec;
struct Library *NewlibBase;
struct Interface *INewlib;

static const char Version[] __attribute__((used)) = "\0$VER: " LIBRARY_ID;

// --

U32
WPA_InterfaceObtain( struct Interface *self )
{
	return ( self->Data.RefCount++ );
}

// --

U32
WPA_InterfaceRelease( struct Interface *self )
{
	return ( self->Data.RefCount-- );
}

// --

static struct Library *
Library_Open( struct LibraryManagerInterface *self, U32 version )
{
	struct Library *base;

	base = self->Data.LibBase;

	if ( version > base->lib_Version )
	{
		return ( NULL );
	}

	base->lib_OpenCnt++;
	base->lib_Flags &= ~LIBF_DELEXP;

	return ( base );
}

// --

static BPTR
Library_Expunge( struct LibraryManagerInterface *self )
{
	struct WPALibraryBase *base;

	BPTR seglist;

	base = (APTR) self->Data.LibBase;

	if ( base->Library.lib_OpenCnt )
	{
		base->Library.lib_Flags |= LIBF_DELEXP;

		return ( 0 );
	}

	seglist = base->SegList;

	Remove( & base->Library.lib_Node );

	DropInterface( INewlib );

	CloseLibrary( NewlibBase );

	DeleteLibrary( & base->Library );

	return ( seglist );
}

// --

static BPTR
Library_Close( struct LibraryManagerInterface *self )
{
	struct Library *base;

	base = self->Data.LibBase;
	base->lib_OpenCnt--;

	if ( ( ! base->lib_OpenCnt ) && ( base->lib_Flags & LIBF_DELEXP ) )
	{
		return ( Library_Expunge( self ) );
	}

	return ( 0 );
}

// --

static struct WPALibraryBase *
Library_Init( struct WPALibraryBase *base, BPTR seglist, struct ExecIFace *exec )
{
	IExec = exec;
	base->SegList = seglist;
	base->Library.lib_Node.ln_Type = NT_LIBRARY;
	base->Library.lib_Node.ln_Name = LIBRARY_NAME;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = LIBRARY_VERSION;
	base->Library.lib_Revision = LIBRARY_REVISION;
	base->Library.lib_IdString = LIBRARY_ID;

	NewlibBase = OpenLibrary( "newlib.library", 50 );

	if ( ! NewlibBase )
	{
		return ( NULL );
	}

	INewlib = GetInterface( NewlibBase, "main", 1, NULL );

	if ( ! INewlib )
	{
		CloseLibrary( NewlibBase );

		NewlibBase = NULL;

		return ( NULL );
	}

	return ( base );
}

// --

static const APTR ManagerVectors[] =
{
	WPA_InterfaceObtain, WPA_InterfaceRelease, NULL, NULL,
	Library_Open, Library_Close, Library_Expunge, NULL, (APTR) -1
};

static const struct TagItem ManagerTags[] =
{
	{ MIT_Name, (Tag) "__library" },
	{ MIT_VectorTable, (Tag) ManagerVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const struct TagItem WPATags[] =
{
	{ MIT_Name, (Tag) "main" },
	{ MIT_VectorTable, (Tag) WPAInterfaceVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const APTR Interfaces[] = { (APTR) ManagerTags, (APTR) WPATags, NULL };

static const struct TagItem InitTags[] =
{
	{ CLT_DataSize, sizeof( struct WPALibraryBase ) },
	{ CLT_Interfaces, (Tag) Interfaces },
	{ CLT_InitFunc, (Tag) Library_Init },
	{ CLT_NoLegacyIFace, TRUE },
	{ TAG_END, 0 }
};

static const struct Resident Resident __attribute__((used)) =
{
	RTC_MATCHWORD, (APTR) & Resident, (APTR) ( & Resident + 1 ),
	RTF_NATIVE | RTF_AUTOINIT, LIBRARY_VERSION, NT_LIBRARY, 0,
	LIBRARY_NAME, LIBRARY_ID, (APTR) InitTags
};

// --

S32
_start( void )
{
	return ( RETURN_FAIL );
}
