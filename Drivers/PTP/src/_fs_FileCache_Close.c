
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void
_fs_FileCache_Close( struct FS_ObjLock *lock )
{
	STR separator;

	if ( ! lock->CacheName )
	{
		return;
	}

	if ( lock->Cache )
	{
		Close( lock->Cache );
	}

	Delete( lock->CacheName );

	separator = strrchr( lock->CacheName, '/' );
	*separator = 0;

	Delete( lock->CacheName );

	FreeVec( lock->CacheName );

	lock->Cache = 0;
	lock->CacheName = NULL;
}

// --
