
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

	ObtainSemaphore( &fs->fs_Semaphore );

	if ( ! fs->fs_Running )
	{
		error = ERROR_DEVICE_NOT_MOUNTED;
	}
	else if ( ! file )
	{
		error = ERROR_REQUIRED_ARG_MISSING;
	}
	else if ( mode == MODE_NEWFILE )
	{
		lock = _fs_File_Create( fs, & error, (PTR) rel, name );
	}
	else if ( ( mode != MODE_OLDFILE ) && ( mode != MODE_READWRITE ) )
	{
		error = ERROR_DISK_WRITE_PROTECTED;
	}
	else
	{
		node = _fs_Node_Locate_Rel( fs, &error, (PTR)rel, name );

		if ( ( ! node ) && ( mode == MODE_READWRITE ) && ( error == ERROR_OBJECT_NOT_FOUND ) )
		{
			lock = _fs_File_Create( fs, &error, (PTR)rel, name );
		}
		else if ( node )
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
	}

	if ( lock )
	{
		file->fh_Arg1 = MKBADDR( lock );
		file->fh_Arg2 = lock;
	}

	ReleaseSemaphore( &fs->fs_Semaphore );

	if ( res2 )
	{
		*res2 = error;
	}

	return ( lock ? DOSTRUE : DOSFALSE );
}

// --
