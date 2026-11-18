
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Lock *_fs__FSParentOfFH( struct FSVP *vp, LONG *res2, struct FileHandle *file )
{
struct FS_Struct *fs;
struct FS_ObjLock *lock;
struct FS_ObjNode *node;
struct Lock *retval;
S32 error;

	MYINFO( "PTP-FS : _fs__FSParentOfFH" );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	retval = NULL;

	if ( file )
	{
		lock = file->fh_Arg2;

		if ( lock )
		{
			node = lock->Node;

			if ( node )
			{
				node = node->on_Parent_dir;

				if ( node )
				{
					retval = (PTR) _fs_Lock_New_From_Node( fs, node, & error, SHARED_LOCK );

					if ( res2 )
					{
						*res2 = error;
					}
				}
			}
		}
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return(	retval );
}

// --

