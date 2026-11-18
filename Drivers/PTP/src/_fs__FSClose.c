
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
_fs__FSClose( struct FSVP *vp, LONG *res2, struct FileHandle *file )
{
	struct FS_Struct *fs;
	struct FS_ObjLock *lock;

	S32 result;
	S32 error;

	fs = vp->FSV.FSPrivate;
	result = DOSFALSE;
	error = ERROR_INVALID_LOCK;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( ( file ) && ( file->fh_Arg2 ) )
	{
		lock = file->fh_Arg2;

		error = lock->Node->on_Pending ? _fs_File_Upload( lock ) : 0;

		result = _fs_Lock_Free( fs, lock );

		if ( result )
		{
			file->fh_Arg1 = 0;
			file->fh_Arg2 = NULL;
		}
		else if ( ! error )
		{
			error = ERROR_INVALID_LOCK;
		}
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	if ( res2 )
	{
		*res2 = error;
	}

	return ( error ? DOSFALSE : DOSTRUE );
}

// --
