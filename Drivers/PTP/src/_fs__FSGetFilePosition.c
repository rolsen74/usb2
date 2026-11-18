
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S64
_fs__FSGetFilePosition( struct FSVP *vp, LONG *res2, struct FileHandle *file )
{
	struct FS_Struct *fs;
	struct FS_ObjLock *lock;

	S64 value;
	S32 error;

	fs = vp->FSV.FSPrivate;
	value = -1;
	error = 0;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( ( file ) && ( file->fh_Arg2 ) )
	{
		lock = file->fh_Arg2;
		value = lock->Position;
	}
	else
	{
		error = ERROR_INVALID_LOCK;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	if ( res2 )
	{
		*res2 = error;
	}

	return ( value );
}

// --
