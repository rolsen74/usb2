
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define LIBRARY_ID USB2WPA2_NAME " 1.0 (19.11.2026)"

struct WPA2Base
{
	struct Library Library;
	BPTR SegList;
};

struct ExecIFace *IExec;
struct Library *NewlibBase;
struct Interface *INewlib;

static const char Version[] __attribute__((used)) = "\0$VER: " LIBRARY_ID;

// --

static U32
Interface_Obtain( struct Interface *self )
{
	return ( self->Data.RefCount++ );
}

// --

static U32
Interface_Release( struct Interface *self )
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
	struct WPA2Base *base;

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

static struct WPA2Base *
Library_Init( struct WPA2Base *base, BPTR seglist, struct ExecIFace *exec )
{
	IExec = exec;
	base->SegList = seglist;
	base->Library.lib_Node.ln_Type = NT_LIBRARY;
	base->Library.lib_Node.ln_Name = USB2WPA2_NAME;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = USB2WPA2_VERSION;
	base->Library.lib_Revision = 0;
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

static const U8 *
Library_Association_IE( struct USB2WPA2IFace *self __attribute__((unused)) )
{
	return ( AssociationRSN );
}

// --

static const APTR ManagerVectors[] =
{
	Interface_Obtain, Interface_Release, NULL, NULL,
	Library_Open, Library_Close, Library_Expunge, NULL, (APTR) -1
};

static const APTR WPAVectors[] =
{
	Interface_Obtain, Interface_Release, NULL, NULL,
	Library_Association_IE, WPA2_Create, WPA2_Destroy, WPA2_Process, (APTR) -1
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
	{ MIT_VectorTable, (Tag) WPAVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const APTR Interfaces[] = { (APTR) ManagerTags, (APTR) WPATags, NULL };

static const struct TagItem InitTags[] =
{
	{ CLT_DataSize, sizeof( struct WPA2Base ) },
	{ CLT_Interfaces, (Tag) Interfaces },
	{ CLT_InitFunc, (Tag) Library_Init },
	{ CLT_NoLegacyIFace, TRUE },
	{ TAG_END, 0 }
};

static const struct Resident Resident __attribute__((used)) =
{
	RTC_MATCHWORD, (APTR) & Resident, (APTR) ( & Resident + 1 ),
	RTF_NATIVE | RTF_AUTOINIT, USB2WPA2_VERSION, NT_LIBRARY, 0,
	USB2WPA2_NAME, LIBRARY_ID, (APTR) InitTags
};

// --

S32
_start( void )
{
	return ( RETURN_FAIL );
}
