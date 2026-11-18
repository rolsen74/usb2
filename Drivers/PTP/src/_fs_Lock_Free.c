
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

int32 _fs_Lock_Free( struct FS_Struct *fs, struct FS_ObjLock *lock )
{
struct FS_ObjLock *parent;
struct FS_ObjLock *next;
struct FS_ObjNode *camera;
S32 retval;

	MYINFO( "PTP-FS : _fs_Lock_Free : %p", lock );

	retval	= FALSE;

	/*
	**  We allow zero (root) locks to succeed.
	*/

	if ( ! lock )
	{
		retval = DOSTRUE;
		goto bailout;
	}

	/*
	**  Get the first node of the lock list.
	*/

	parent = BADDR(fs->fs_LockList);

	/*
	**  We need to disconnect this lock from the locklist.
	**
	**  For a head case only, just stick the locks
	**  fl_link (next) BPTR as the new lock_list head BPTR,
	**  all other locks must be hanging of this one.
	**  Else, -- Scan through the list to find it.
	*/

	if ( lock == parent )
	{
		fs->fs_LockList = lock->Lock.fl_Link;
	}
	else
	{
		while( parent )
		{
			next = BADDR(parent->Lock.fl_Link);

			if ( next == lock )
			{
				break;
			}

			parent = next;
		}

		/*
		**  Better check if the lock is actually in our list.
		**  We're in trouble if it isn't, it may have been freed before.
		*/

		if ( ! parent )
		{
			MYERROR( "PTP-FS : Warning no lock" );
			goto bailout;
		}

		/*
		**  Remove 'lock' from list by setting
		**  the parents->next field to this locks
		**  next field.  (can also be 0).
		*/

		parent->Lock.fl_Link = lock->Lock.fl_Link;
	}

	_fs_FileCache_Close( lock );

	camera = lock->Node->on_CameraRoot;

	if ( lock->Node->on_Pending )
	{
		Remove( & lock->Node->on_Node );

		_fs_Node_Delete( fs, lock->Node );
	}

	if ( ( camera ) && ( ! --camera->on_References ) && ( ! camera->on_Camera ) )
	{
		_fs_Node_Delete( fs, camera );
	}

	/*
	**  Null out fl_Task to avoid stale lock problems with pools.
	**  It's no guarantee, but it might help.
	*/

	lock->Lock.fl_Port = NULL;
	lock->Node = NULL;

	FreeDosObject( DOS_LOCK, lock );

	retval = DOSTRUE;

bailout:

	return( retval );
}

// --

