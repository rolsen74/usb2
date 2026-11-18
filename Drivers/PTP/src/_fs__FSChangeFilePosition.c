
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
_fs__FSChangeFilePosition( struct FSVP *vp, LONG *res2, struct FileHandle *file, LONG mode, S64 position )
{
	struct FS_Struct *fs;
	struct FS_ObjLock *lock;

	S64 base;
	S32 error;

	fs = vp->FSV.FSPrivate;
	lock = NULL;
	error = 0;
	base = 0;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( ( ! file ) || ( ! file->fh_Arg2 ) )
	{
		error = ERROR_INVALID_LOCK;
	}
	else
	{
		lock = file->fh_Arg2;

		switch( mode )
		{
			case OFFSET_BEGINNING:
				break;
			case OFFSET_CURRENT:
				base = lock->Position;
				break;
			case OFFSET_END:
				base = lock->Node->on_File_Size;
				break;
			default:
				error = ERROR_SEEK_ERROR;
				break;
		}

		if ( ( position < -base ) || ( position > lock->Node->on_File_Size - base ) )
		{
			error = ERROR_SEEK_ERROR;
		}

		if ( ! error )
		{
			lock->Position = base + position;
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
