
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --
// -- Manager Interface

#include "_man_Init.c"

#include "_man_AbortIO.c"
#include "_man_BeginIO.c"

#include "_man_Obtain.c"
#include "_man_Release.c"
#include "_man_Expunge.c"
#include "_man_Open.c"
#include "_man_Close.c"

// --

static const PTR _manager_Vectors[] =
{
	(PTR) _manager_Obtain,
	(PTR) _manager_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) _manager_Open,
	(PTR) _manager_Close,
	(PTR) _manager_Expunge,
	(PTR) NULL,
	(PTR) _manager_BeginIO,
	(PTR) _manager_AbortIO,
	(PTR) -1
};

static const struct TagItem _manager_Tags[] =
{
	{ MIT_Name,			(Tag) "__device"		},
	{ MIT_VectorTable,	(Tag) _manager_Vectors	},
	{ MIT_Version,		(Tag) 1					},
	{ TAG_END,			(Tag) 0					}
};

// --

const PTR devInterfaces[] =
{
	(PTR) _manager_Tags,
	(PTR) NULL
};

// --

static USED char verstag[] = VERSTAG;

// --

static USED const struct Resident dev_res =
{
/* rt_MatchWord	*/			RTC_MATCHWORD,
/* rt_MatchTag	*/ (PTR)	( & dev_res + 0 ),
/* rt_EndSkip	*/ (PTR)	( & dev_res + 1 ),
/* rt_Flags		*/			RTF_NATIVE|RTF_COLDSTART,
/* rt_Version	*/			VERSION,
/* rt_Type		*/			0,
/* rt_Pri		*/			1,
/* rt_Name		*/ (STR)	DEVNAME,
/* rt_IdString	*/ (STR)	VSTRING,
/* rt_Init		*/ (PTR)	_manager_Init
};

// --

S32 _start( STR argstring UNUSED, S32 arglen UNUSED, struct ExecBase *sysbase UNUSED )
{
	return( 0 );
}

// --
