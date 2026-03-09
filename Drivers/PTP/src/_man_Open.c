
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

PTR _man_Open( struct LibraryManagerInterface *Self, U32 version UNUSED )
{
struct PTPBase *libBase;

	MYDEBUG( "PTP : _man_Open" );

	libBase = (PTR) Self->Data.LibBase;

	libBase->lib_Base.lib_OpenCnt++;
	libBase->lib_Base.lib_Flags &= ~LIBF_DELEXP;

	return( (PTR) libBase );
}

// --
