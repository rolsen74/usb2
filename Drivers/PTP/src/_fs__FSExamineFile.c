
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct ExamineData *_fs__FSExamineFile( struct FSVP *vp, int32 *res2, struct FileHandle *fh )
{
struct FS_Struct *fs;
struct ExamineData *retval;

	MYINFO( "PTP-FS : _fs__FSExamineDir" );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( fh )
	{
		retval = _fs__FSExamineLock( vp, res2, fh->fh_Arg2 );
	}
	else
	{
		if ( res2 )
		{
			*res2 = ERROR_REQUIRED_ARG_MISSING;
		}

		retval = NULL;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return(	retval );
}

// --

