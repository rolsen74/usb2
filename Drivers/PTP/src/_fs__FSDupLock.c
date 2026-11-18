
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Lock *_fs__FSDupLock( struct FSVP *vp, LONG *res2, struct Lock *lockin )
{
struct FS_Struct *fs;
struct FS_ObjLock *lock;
PTR retval;
S32 error;

	MYINFO( "PTP-FS : _fs__FSDupLock" );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( lockin )
	{
		lock = (PTR) lockin;
		error = 0;

		retval = _fs_Lock_New_From_Node( fs, lock->Node, & error, SHARED_LOCK );

		if ( res2 )
		{
			*res2 = error;
		}
	}
	else
	{
		retval = NULL;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return(	retval );
}

// --
