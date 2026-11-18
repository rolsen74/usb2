
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Version.h"

// --

static CONST_APTR _manager_Vectors[] =
{
	(APTR) Device_Obtain,
	(APTR) Device_Release,
	(APTR) NULL,
	(APTR) NULL,
	(APTR) Device_Open,
	(APTR) Device_Close,
	(APTR) Device_Expunge,
	(APTR) NULL,
	(APTR) Device_BeginIO,
	(APTR) Device_AbortIO,
	(APTR) -1
};

static struct TagItem _manager_Tags[] =
{
	{ MIT_Name,			(Tag) "__device"		},
	{ MIT_VectorTable,	(Tag) _manager_Vectors	},
	{ MIT_Version,		(Tag) 1					},
	{ TAG_END,			(Tag) 0					}
};

static CONST_APTR devInterfaces[] =
{
	(APTR) _manager_Tags,
	(APTR) NULL
};

static struct TagItem devCreateTags[] =
{
	{ CLT_DataSize,		(Tag) sizeof( struct devBase )	},
	{ CLT_Interfaces,	(Tag) devInterfaces				},
	{ CLT_InitFunc,		(Tag) Device_Init				},
	{ TAG_END,			(Tag) 0							}
};

static USED struct Resident dev_res =
{
/* rt_MatchWord */						RTC_MATCHWORD,
/* rt_MatchTag  */	(struct Resident *)	( & dev_res + 0 ),
/* rt_EndSkip   */	(APTR)				( & dev_res + 1 ),
/* rt_Flags     */						RTF_NATIVE|RTF_AUTOINIT|RTF_COLDSTART,
/* rt_Version   */						VERSION,
/* rt_Type      */						NT_DEVICE,
/* rt_Pri       */						0,
/* rt_Name      */	(STR)			DEVNAME,
/* rt_IdString  */	(STR)			VSTRING,
/* rt_Init      */	(APTR)				devCreateTags
};

// --

static TEXT USED 		verstag[] 		= VERSTAG;

struct ExecIFace *			IExec			= NULL;

struct Library *			NewlibBase		= NULL;
struct Interface *			INewlib			= NULL;

struct Library *			DOSBase			= NULL;
struct DOSIFace *			IDOS			= NULL;

struct Library *			UtilityBase		= NULL;
struct UtilityIFace *		IUtility		= NULL;

struct TimerIFace *			ITimer			= NULL;

// --
