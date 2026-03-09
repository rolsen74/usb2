
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

int32 _fs__FSUnLock( struct FSVP *vp, int32 *res2, struct Lock *lock )
{
struct PTP_FSStruct *fs;
int32 retval;

	MYINFO( "PTP-FS : _fs__FSUnLock" );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	retval = _fs_Lock_Free( fs, (PTR) lock );

	if ( res2 )
	{
		*res2 = 0;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return( retval );
}

// --

