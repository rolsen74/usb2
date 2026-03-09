
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Lock *_fs__FSLock( struct FSVP *vp, int32 *res2, struct Lock *rel_lock, CONST_STRPTR name, int32 mode )
{
struct PTP_FSStruct *fs;
struct FS_ObjNode *obj;
PTR retval;

	MYINFO( "PTP-FS : _fs__FSLock '%s'", name );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	/*
	** Find the object node by 'name' at the level of 'rel_lock' (following links)
	*/

	obj = _fs_Node_Locate_Rel( fs, NULL, (PTR) rel_lock, FOLLOW_LINKS, res2, (PTR) name );

	if ( obj )
	{
		retval = _fs_Node_NewLock( fs, obj, res2, mode );
	}
	else
	{
		retval = NULL;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return( retval );
}

// --
