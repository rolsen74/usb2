
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <exec/resident.h>
#include "All.h"

// --

#define DRIVER_NAME "cls_Bluetooth.Driver"
#define DRIVER_REVISION 27
#define DRIVER_ID DRIVER_NAME " 1.27 (27.11.2026)"

// --

struct ExecIFace *IExec;

struct BluetoothDriverBase
{
	struct Library Library;
	BPTR SegList;
};

static const char VersionTag[] __attribute__((used)) = "\0$VER: " DRIVER_ID;

// --

static U32
Obtain( struct Interface *self )
{
	return ( ++self->Data.RefCount );
}

// --

static U32
Release( struct Interface *self )
{
	return ( --self->Data.RefCount );
}

// --

static struct Library *
Open( struct LibraryManagerInterface *self, U32 version )
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
Expunge( struct LibraryManagerInterface *self )
{
	struct BluetoothDriverBase *base;

	BPTR seglist;

	base = (PTR) self->Data.LibBase;

	if ( base->Library.lib_OpenCnt )
	{
		base->Library.lib_Flags |= LIBF_DELEXP;

		return ( 0 );
	}

	seglist = base->SegList;

	Remove( & base->Library.lib_Node );

	DeleteLibrary( & base->Library );

	return ( seglist );
}

// --

static BPTR
Close( struct LibraryManagerInterface *self )
{
	struct Library *base;

	base = self->Data.LibBase;
	base->lib_OpenCnt--;

	if ( ( ! base->lib_OpenCnt ) && ( base->lib_Flags & LIBF_DELEXP ) )
	{
		return ( Expunge( self ) );
	}

	return ( 0 );
}

// --

static struct BluetoothDriverBase *
Init( struct BluetoothDriverBase *base, BPTR seglist, struct ExecIFace *exec )
{
	IExec = exec;
	base->SegList = seglist;
	base->Library.lib_Node.ln_Type = NT_LIBRARY;
	base->Library.lib_Node.ln_Name = DRIVER_NAME;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = 1;
	base->Library.lib_Revision = DRIVER_REVISION;
	base->Library.lib_IdString = DRIVER_ID;

	return ( base );
}

// --

static const PTR ManagerVectors[] =
{
	(PTR) Obtain, (PTR) Release, NULL, NULL,
	(PTR) Open, (PTR) Close, (PTR) Expunge, NULL, (PTR) -1
};

static const PTR MainVectors[] =
{
	(PTR) Obtain, (PTR) Release, NULL, NULL,
	(PTR) BTUSB_Entry, (PTR) -1
};

static const struct TagItem ManagerTags[] =
{
	{ MIT_Name, (Tag) "__library" },
	{ MIT_VectorTable, (Tag) ManagerVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const struct TagItem MainTags[] =
{
	{ MIT_Name, (Tag) "main" },
	{ MIT_VectorTable, (Tag) MainVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const PTR Interfaces[] = { (PTR) ManagerTags, (PTR) MainTags, NULL };

static const struct TagItem InitTags[] =
{
	{ CLT_DataSize, sizeof( struct BluetoothDriverBase ) },
	{ CLT_Interfaces, (Tag) Interfaces },
	{ CLT_InitFunc, (Tag) Init },
	{ CLT_NoLegacyIFace, TRUE },
	{ TAG_END, 0 }
};

static struct Resident DriverResident __attribute__((used)) =
{
	.rt_MatchWord = RTC_MATCHWORD,
	.rt_MatchTag = (PTR) & DriverResident,
	.rt_EndSkip = (PTR) ( & DriverResident + 1 ),
	.rt_Flags = RTF_NATIVE | RTF_AUTOINIT,
	.rt_Version = 1,
	.rt_Type = NT_LIBRARY,
	.rt_Name = DRIVER_NAME,
	.rt_IdString = DRIVER_ID,
	.rt_Init = (PTR) InitTags
};

// --

S32
_start( void )
{
	return ( RETURN_FAIL );
}

// --
