
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Lock *_fs__FSParentOfFH( struct FSVP *vp, int32 *res2, struct FileHandle *file )
{
struct PTP_FSStruct *fs;
struct FS_ObjLock *lock;
struct FS_ObjNode *node;
struct Lock *retval;

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
				node = node->Parent_dir;

				if ( node )
				{
					retval = (PTR) _fs_Node_NewLock( fs, node, res2, SHARED_LOCK );
				}
			}
		}
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return(	retval );
}

// --

