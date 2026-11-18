
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
_fs__FSWrite( struct FSVP *vp, LONG *res2, struct FileHandle *file, STR buffer, LONG count )
{
	struct FS_Struct *fs;
	struct FS_ObjLock *lock;
	struct FS_ObjNode *node;

	S32 error;
	S32 written;

	fs = vp->FSV.FSPrivate;
	error = 0;
	written = -1;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( ( ! file ) || ( ! file->fh_Arg2 ) || ( count < 0 ) || ( count && ! buffer ) )
	{
		error = ERROR_REQUIRED_ARG_MISSING;
		goto done;
	}

	lock = file->fh_Arg2;
	node = lock->Node;

	if ( ! node->on_Pending )
	{
		error = ERROR_WRITE_PROTECTED;
		goto done;
	}

	if ( ! node->on_CameraRoot->on_Camera )
	{
		error = ERROR_DEVICE_NOT_MOUNTED;
		goto failed;
	}

	if ( lock->WriteError )
	{
		error = lock->WriteError;
		goto done;
	}

	if ( (U64) count > PTP_MAX_UPLOAD - lock->Position )
	{
		error = ERROR_OBJECT_TOO_LARGE;
		goto failed;
	}

	if ( ! count )
	{
		written = 0;
		goto done;
	}

	if ( ! ChangeFilePosition( lock->Cache, lock->Position, OFFSET_BEGINNING ) )
	{
		error = IoErr();

		if ( ! error )
		{
			error = ERROR_SEEK_ERROR;
		}

		goto failed;
	}

	written = Write( lock->Cache, buffer, count );

	if ( written != count )
	{
		error = IoErr();

		if ( ! error )
		{
			error = ERROR_DISK_FULL;
		}

		goto failed;
	}

	lock->Position += written;
	node->on_File_Size = MAX( node->on_File_Size, (S64) lock->Position );
	goto done;

failed:

	// A failed local write must never result in a truncated upload on Close().
	lock->WriteError = error;

done:

	ReleaseSemaphore( & fs->fs_Semaphore );

	if ( res2 )
	{
		*res2 = error;
	}

	return ( error ? -1 : written );
}

// --
