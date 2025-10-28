
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
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
