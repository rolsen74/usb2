/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Library *SysBase = NULL;
struct ExecIFace *IExec = NULL;
struct USB2IFace *IUSB2 = NULL;

// --

static const PTR Driver_Manager_Vectors[] =
{
	(PTR) Driver_Obtain,
	(PTR) Driver_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) Driver_Open,
	(PTR) Driver_Close,
	(PTR) Driver_Expunge,
	(PTR) NULL,
	(PTR) -1
};

static const struct TagItem Driver_Manager_Tags[] =
{
	{ MIT_Name, (Tag) "__library" },
	{ MIT_VectorTable, (Tag) Driver_Manager_Vectors },
	{ MIT_Version, (Tag) 1 },
	{ TAG_END, (Tag) 0 }
};

static const PTR Driver_USB_Vectors[] =
{
	(PTR) Driver_USB_Obtain,
	(PTR) Driver_USB_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) Audio_Entry,
	(PTR) -1
};

static const struct TagItem Driver_USB_Tags[] =
{
	{ MIT_Name, (Tag) "main" },
	{ MIT_VectorTable, (Tag) Driver_USB_Vectors },
	{ MIT_Version, (Tag) 1 },
	{ TAG_END, (Tag) 0 }
};

const PTR DriverInterfaces[] =
{
	(PTR) Driver_Manager_Tags,
	(PTR) Driver_USB_Tags,
	(PTR) NULL
};

static USED const char verstag[] = "\0$VER: " AUDIO_DRIVER_LABEL;

static USED const struct Resident driver_res =
{
	RTC_MATCHWORD,
	(PTR) & driver_res,
	(PTR) ( & driver_res + 1 ),
	RTF_NATIVE | RTF_COLDSTART,
	AUDIO_DRIVER_VERSION,
	NT_LIBRARY,
	0,
	(STR) AUDIO_DRIVER_NAME,
	(STR) AUDIO_DRIVER_LABEL,
	(PTR) Driver_Init
};

// --

S32 _start( STR argstring UNUSED, S32 arglen UNUSED, struct ExecBase *sysbase UNUSED )
{
	return ( 0 );
}

// --
