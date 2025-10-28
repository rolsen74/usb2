
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
