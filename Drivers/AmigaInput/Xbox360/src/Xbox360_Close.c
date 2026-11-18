
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
Xbox360_Close( struct LibraryManagerInterface *Self )
{
	struct Xbox360Base *base;

	base = (PTR) Self->Data.LibBase;
	base->Library.lib_OpenCnt--;

	if ( ( ! base->Library.lib_OpenCnt ) && ( base->Library.lib_Flags & LIBF_DELEXP ) )
	{
		return ( Xbox360_Expunge( Self ) );
	}

	return ( NULL );
}

// --
