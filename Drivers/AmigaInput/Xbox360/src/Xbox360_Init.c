
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Xbox360.h"

// --

struct Xbox360Base *
Xbox360_Init( PTR unused UNUSED, PTR seglist, struct ExecBase *sysbase )
{
	struct Xbox360Base *base;

	IExec = (PTR) sysbase->MainInterface;

	base = (PTR) CreateLibraryTags(
		CLT_DataSize, sizeof( struct Xbox360Base ),
		CLT_Interfaces, Xbox360_Interfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END );

	if ( ! base )
	{
		return ( NULL );
	}

	base->Library.lib_Node.ln_Type = NT_LIBRARY;
	base->Library.lib_Node.ln_Name = (STR) XBOX360_DRIVER;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = XBOX360_VERSION;
	base->Library.lib_Revision = XBOX360_REVISION;
	base->Library.lib_IdString = (STR) XBOX360_ID;
	base->SegList = seglist;

	AddLibrary( (PTR) base );

	return ( base );
}

// --
