
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Lock *_fs__FSDupLock( struct FSVP *vp, int32 *res2, struct Lock *lockin )
{
struct FS_Struct *fs;
struct FS_ObjLock *lock;
PTR retval;

	MYINFO( "PTP-FS : _fs__FSDupLock" );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( lockin )
	{
		lock = (PTR) lockin;

		retval = _fs_Lock_New_From_Node( fs, lock->Node, res2, SHARED_LOCK );
	}
	else
	{
		retval = NULL;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return(	retval );
}

// --
