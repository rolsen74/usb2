
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static const PTR _manager_Vectors[] =
{
	(PTR) _man_Obtain,
	(PTR) _man_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) _man_Open,
	(PTR) _man_Close,
	(PTR) _man_Expunge,
	(PTR) NULL,
	(PTR) -1
};

static const struct TagItem _manager_Tags[] =
{
	{ MIT_Name,			(Tag) "__library"		},
	{ MIT_VectorTable,	(Tag) _manager_Vectors	},
	{ MIT_Version,		(Tag) 1					},
	{ TAG_END,			(Tag) 0					}
};

// --

static const PTR _main_Vectors[] =
{
	(PTR) _usb_Obtain,
	(PTR) _usb_Release,
	(PTR) NULL,	// _main_Expunge,
	(PTR) NULL,	// _main_Clone,
	(PTR) _usb_Entry,
	(PTR) -1
};

static const struct TagItem _main_Tags[] =
{
	{ MIT_Name,			(Tag) "main"			},
	{ MIT_VectorTable,	(Tag) _main_Vectors		},
//	{ MIT_DataSize,		(Tag) sizeof( struct PTPData )},
//	{ MIT_Flags,		(Tag) IFLF_PRIVATE		},
	{ MIT_Version,		(Tag) 1					},
	{ TAG_END,			(Tag) 0					}
};

// --

const PTR libInterfaces[] =
{
	(PTR) _manager_Tags,
	(PTR) _main_Tags,
	(PTR) NULL
};

// --
// if you want you can add a litte extra info for version
// string if you want.. but that is optional
// maybe a copyright line?

static USED char	verstag[] = VERSTAG;
//static USED char	verstag[] = VERSTAG "Coded by awesome me";

// --

static USED const struct Resident lib_res =
{
/* rt_MatchWord	*/			RTC_MATCHWORD,
/* rt_MatchTag	*/ (PTR)	( & lib_res + 0 ),
/* rt_EndSkip	*/ (PTR)	( & lib_res + 1 ),
/* rt_Flags		*/			RTF_NATIVE|RTF_COLDSTART,
/* rt_Version	*/			VERSION,
/* rt_Type		*/			NT_LIBRARY,
/* rt_Pri		*/			LIBPRI,
/* rt_Name		*/ (STR)	LIBNAME,
/* rt_IdString	*/ (STR)	VSTRING,
/* rt_Init		*/ (PTR)	_man_Init
};

// --
// This is the entry point, if someone tries to run us from shell/wb

S32 _start( STR argstring UNUSED, S32 arglen UNUSED, struct ExecBase *sysbase UNUSED )
{
	return( 0 );
}

// --
