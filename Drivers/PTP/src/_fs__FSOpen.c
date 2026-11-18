
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
_fs__FSOpen( struct FSVP *vp, LONG *res2, struct FileHandle *file, struct Lock *rel, const char *filename, LONG mode )
{
	struct FS_Struct *fs;
	struct FS_ObjNode *node;
	struct FS_ObjLock *lock;

	STR name;
	S32 error;

	fs = vp->FSV.FSPrivate;
	name = (STR) filename;
	lock = NULL;
	error = 0;

	if ( ! file )
	{
		error = ERROR_REQUIRED_ARG_MISSING;
	}
	else if ( mode == MODE_NEWFILE || mode == MODE_READWRITE )
	{
		ObtainSemaphore( & fs->fs_Semaphore );

		lock = _fs_File_Create( fs, & error, (PTR) rel, name );

		ReleaseSemaphore( & fs->fs_Semaphore );
	}
	else if ( mode != MODE_OLDFILE )
	{
		error = ERROR_DISK_WRITE_PROTECTED;
	}
	else
	{
		ObtainSemaphore( & fs->fs_Semaphore );

		node = _fs_Node_Locate_Rel( fs, & error, (PTR) rel, name );

		if ( node )
		{
			if ( node->on_Type == FSO_TYPE_FILE )
			{
				lock = _fs_Lock_New_From_Node( fs, node, & error, SHARED_LOCK );
			}
			else
			{
				error = ERROR_OBJECT_WRONG_TYPE;
			}
		}

		ReleaseSemaphore( & fs->fs_Semaphore );
	}

	if ( lock )
	{
		file->fh_Arg1 = MKBADDR( lock );
		file->fh_Arg2 = lock;
	}

	if ( res2 )
	{
		*res2 = error;
	}

	return ( lock ? DOSTRUE : DOSFALSE );
}

// --
