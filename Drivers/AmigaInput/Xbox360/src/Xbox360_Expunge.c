
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Xbox360.h"

// --

PTR
Xbox360_Expunge( struct LibraryManagerInterface *Self )
{
	struct Xbox360Base *base;

	PTR seglist;

	base = (PTR) Self->Data.LibBase;

	if ( base->Library.lib_OpenCnt )
	{
		base->Library.lib_Flags |= LIBF_DELEXP;

		return ( NULL );
	}

	seglist = base->SegList;

	Remove( (PTR) base );

	DeleteLibrary( (PTR) base );

	return ( seglist );
}

// --
