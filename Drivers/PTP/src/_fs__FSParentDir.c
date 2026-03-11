
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Lock *_fs__FSParentDir( struct FSVP *vp, int32 *res2, struct Lock *dirlock )
{
struct FS_Struct *fs;
struct FS_ObjLock  *lock;
struct FS_ObjNode *node;
struct Lock *retval;

	MYINFO( "PTP-FS : _fs__FSParentDir" );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	retval = NULL;

	if ( res2 )
	{
		*res2 = 0;
	}

	if ( dirlock )
	{
		lock = (PTR) dirlock;

		node = lock->Node;

		if ( node )
		{
			node = node->on_Parent_dir;

			if ( node )
			{
				retval = (PTR) _fs_Lock_New_From_Node( fs, node, res2, SHARED_LOCK );
			}
		}
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return(	retval );
}

// --

