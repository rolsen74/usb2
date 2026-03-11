
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Lock *_fs__FSDupLockFromFH( struct FSVP *vp, int32 *res2, struct FileHandle *file )
{
struct FS_Struct *fs;
struct Lock *retval;

	MYINFO( "PTP-FS : _fs__FSDupLockFromFH" );
	
	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( file )
	{
		retval = _fs__FSDupLock( vp, res2, file->fh_Arg2 );
	}
	else
	{
		retval = NULL;

		if ( res2 )
		{
			*res2 = ERROR_REQUIRED_ARG_MISSING;
		}
	}
	
	ReleaseSemaphore( & fs->fs_Semaphore );

	return( retval );
}

// --

