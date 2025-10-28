
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

PTR _manager_Expunge( struct LibraryManagerInterface *Self )
{
struct libBase *libBase;
PTR retval;

	libBase = (PTR) Self->Data.LibBase;

	if ( ! libBase->lib_Base.lib_OpenCnt )
	{
		retval = libBase->lib_SegList;

		Resources_Cleanup( libBase );

		Remove( (PTR) libBase );
		DeleteLibrary( (PTR) libBase );
	}
	else
	{
		libBase->lib_Base.lib_Flags |= LIBF_DELEXP;

		retval = NULL;
	}

	return( retval );
}

// --
