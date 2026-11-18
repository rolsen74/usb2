
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct ExamineData *
_fs__FSExamineLock( struct FSVP *vp, LONG *res2, struct Lock *objlock )
{
	struct FS_Struct *fs;
	struct FS_ObjLock *lock;
	struct FS_ObjNode *node;
	struct ExamineData *data;

	S32 error;

	fs = vp->FSV.FSPrivate;
	lock = (PTR) objlock;
	data = NULL;
	error = 0;

	ObtainSemaphore( & fs->fs_Semaphore );

	node = lock ? lock->Node : fs->fs_RootNode;

	if ( ! _fs_Node_Online( node ) )
	{
		error = ERROR_DEVICE_NOT_MOUNTED;
	}
	else
	{
		data = _fs_ExamineNode( node, NULL );

		if ( ! data )
		{
			error = ERROR_NO_FREE_STORE;
		}
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	if ( res2 )
	{
		*res2 = error;
	}

	return ( data );
}

// --
