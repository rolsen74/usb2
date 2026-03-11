
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct Lock *_fs__FSLock( struct FSVP *vp, int32 *res2, struct Lock *rel_ptr, CONST_STRPTR name, int32 mode )
{
struct FS_Struct *fs;
struct FS_ObjNode *obj;
struct FS_ObjLock *rel;
PTR retval;

	MYINFO( "PTP-FS : _fs__FSLock '%s'", name );

	rel = (PTR) rel_ptr;

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	obj = _fs_Handle_Rel_Args( fs, rel, name );

	retval = _fs_Lock_New_From_Node( fs, obj, res2, mode );

	DebugPrintF( "New Lock : %p\n", retval );

	ReleaseSemaphore( & fs->fs_Semaphore );

	return( retval );
}

// --
