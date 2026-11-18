
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Xbox360.h"

// --

struct ExecIFace *IExec;

// --

static const PTR Manager_Vectors[] =
{
	(PTR) Xbox360_Obtain,
	(PTR) Xbox360_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) Xbox360_Open,
	(PTR) Xbox360_Close,
	(PTR) Xbox360_Expunge,
	(PTR) NULL,
	(PTR) -1
};

static const struct TagItem Manager_Tags[] =
{
	{ MIT_Name, (Tag) "__library" },
	{ MIT_VectorTable, (Tag) Manager_Vectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

// --

static const PTR USB_Vectors[] =
{
	(PTR) Xbox360_Obtain,
	(PTR) Xbox360_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) Xbox360_Entry,
	(PTR) -1
};

static const struct TagItem USB_Tags[] =
{
	{ MIT_Name, (Tag) "main" },
	{ MIT_VectorTable, (Tag) USB_Vectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

const PTR Xbox360_Interfaces[] =
{
	(PTR) Manager_Tags,
	(PTR) USB_Tags,
	(PTR) NULL
};

// --

static const char version[] __attribute__((used)) = "\0$VER: " XBOX360_ID;

static const struct Resident resident __attribute__((used)) =
{
	RTC_MATCHWORD,
	(PTR) & resident,
	(PTR) ( & resident + 1 ),
	RTF_NATIVE | RTF_COLDSTART,
	XBOX360_VERSION,
	NT_LIBRARY,
	0,
	(STR) XBOX360_DRIVER,
	(STR) XBOX360_ID,
	(PTR) Xbox360_Init
};

// --
