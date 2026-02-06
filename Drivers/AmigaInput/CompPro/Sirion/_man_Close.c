
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

PTR _manager_Close( struct LibraryManagerInterface *Self )
{
struct libBase *libBase;
PTR retval;

	libBase = (PTR) Self->Data.LibBase;
	libBase->lib_Base.lib_OpenCnt--;

	retval = NULL;

	if ( libBase->lib_Base.lib_OpenCnt == 0 )
	{
		if ( libBase->lib_Base.lib_Flags & LIBF_DELEXP )
		{
			retval = _manager_Expunge( Self );
		}
	}

	return( retval );
}

// --
