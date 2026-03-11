
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

int32 _fs__FSUnLock( struct FSVP *vp, int32 *res2, struct Lock *lock_ptr )
{
struct FS_Struct *fs;
struct FS_ObjLock *lock;
int32 retval;

	lock = (PTR) lock_ptr;

	MYINFO( "PTP-FS : _fs__FSUnLock : %p", lock );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	retval = _fs_Lock_Free( fs, lock );

	if ( res2 )
	{
		*res2 = 0;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return( retval );
}

// --

