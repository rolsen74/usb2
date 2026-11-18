
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Lock *_fs__FSLock( struct FSVP *vp, LONG *res2, struct Lock *rel_ptr, const char *name, LONG mode )
{
struct FS_Struct *fs;
struct FS_ObjNode *obj;
struct FS_ObjLock *rel;
PTR retval;
S32 error;

	MYINFO( "PTP-FS : _fs__FSLock '%s'", name );

	rel = (PTR) rel_ptr;

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	obj = _fs_Node_Locate_Rel( fs, & error, rel, (STR) name );
	retval = NULL;

	if ( obj )
	{
		retval = _fs_Lock_New_From_Node( fs, obj, & error, mode );
	}

	if ( res2 )
	{
		*res2 = error;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return( retval );
}

// --
