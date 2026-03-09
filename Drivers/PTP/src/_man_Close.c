
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

PTR _man_Close( struct LibraryManagerInterface *Self )
{
struct PTPBase *libBase;
PTR seglist;

	MYDEBUG( "PTP : _man_Close" );

	seglist = NULL;

	libBase = (PTR) Self->Data.LibBase;

	// Update Library open count
	if ( libBase->lib_Base.lib_OpenCnt )
	{
		libBase->lib_Base.lib_OpenCnt--;
	}

	// Check if we can expunge our self
	if ( ! libBase->lib_Base.lib_OpenCnt )
	{
		if ( libBase->lib_Base.lib_Flags & LIBF_DELEXP )
		{
			seglist = _man_Expunge( Self );
		}
	}

	// Return NULL or SegList
	return( seglist );
}

// --