
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// -- Manager Interface

static const PTR Manager_Vectors[] =
{
	(PTR) _manager_Obtain,
	(PTR) _manager_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) _manager_Open,
	(PTR) _manager_Close,
	(PTR) _manager_Expunge,
	(PTR) NULL,
	(PTR) -1,
};

static const struct TagItem Manager_Tags[] =
{
	{ MIT_Name,				(U32) "__library"		},
	{ MIT_VectorTable,		(U32) Manager_Vectors	},
	{ MIT_Version,			(U32) 1					},
	{ TAG_END,				(U32) 0					}
};

// -- Main Interface

static const PTR Main_Vectors[] =
{
	(PTR) _main_Obtain,
	(PTR) _main_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) _main_GetAttrsA,
	(PTR) _main_GetAttrs,
	(PTR) _main_RunFunction,
	(PTR) _main_RunInterface,
	(PTR) -1
};

static const struct TagItem Main_Tags[] =
{
	{ MIT_Name,				(U32) "main"		},
	{ MIT_VectorTable,		(U32) Main_Vectors	},
	{ MIT_Version,			(U32) 1				},
	{ TAG_END,				(U32) 0				}
};

// --

static const PTR libInterfaces[] =
{
	(PTR) Manager_Tags,
	(PTR) Main_Tags,
	(PTR) NULL
};

static const struct TagItem libCreateTags[] =
{
	{ CLT_DataSize,			(U32) sizeof(struct libBase)	},
	{ CLT_InitFunc,			(U32) _manager_Init				},
	{ CLT_Interfaces,		(U32) libInterfaces				},
	{ CLT_NoLegacyIFace,	(U32) TRUE						},
	{ TAG_DONE,				(U32) 0							}
};

static USED const struct Resident lib_res =
{
/* rt_MatchWord	*/			RTC_MATCHWORD,
/* rt_MatchTag	*/ (PTR)	( & lib_res + 0 ),
/* rt_EndSkip	*/ (PTR)	( & lib_res + 1 ),
/* rt_Flags		*/			RTF_NATIVE|RTF_AUTOINIT|RTF_COLDSTART,
/* rt_Version	*/			VERSION,
/* rt_Type		*/			NT_LIBRARY,
/* rt_Pri		*/			0,
/* rt_Name		*/ (STR)	"CompPro.usbfd",
/* rt_IdString	*/ (STR)	VSTRING,
/* rt_Init		*/ (PTR)	libCreateTags
};

// --
