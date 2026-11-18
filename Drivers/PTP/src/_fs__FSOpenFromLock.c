
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

LONG
_fs__FSOpenFromLock( struct FSVP *vp, LONG *res2, struct FileHandle *file, struct Lock *objlock )
{
	struct FS_Struct *fs;
	struct FS_ObjLock *lock;

	S32 error;

	fs = vp->FSV.FSPrivate;
	lock = (PTR) objlock;
	error = 0;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( ( ! file ) || ( ! lock ) )
	{
		error = ERROR_REQUIRED_ARG_MISSING;
	}
	else if ( lock->Node->on_Type != FSO_TYPE_FILE )
	{
		error = ERROR_OBJECT_WRONG_TYPE;
	}
	else if ( ! _fs_Node_Online( lock->Node ) )
	{
		error = ERROR_DEVICE_NOT_MOUNTED;
	}
	else
	{
		// DOS transfers ownership of the lock on success.
		file->fh_Arg1 = MKBADDR( lock );
		file->fh_Arg2 = lock;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	if ( res2 )
	{
		*res2 = error;
	}

	return ( error ? DOSFALSE : DOSTRUE );
}

// --
