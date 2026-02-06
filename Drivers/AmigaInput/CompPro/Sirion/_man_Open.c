
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

PTR _manager_Open( struct LibraryManagerInterface *Self, U32 version UNUSED )
{
struct libBase *libBase;

	libBase = (PTR) Self->Data.LibBase;
	libBase->lib_Base.lib_OpenCnt++;
	libBase->lib_Base.lib_Flags &= ~LIBF_DELEXP;

	return( libBase );
}

// --
