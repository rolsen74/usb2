
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

/*
**  Given an object node and an access mode (SHARED_LOCK or EXCLUSIVE_LOCK)
**  This will attempt to create a new lock to the object node, if possible.
**  The usual access collision detection is employed.
**  Note: Directories must only be shared locks no matter what is requested.  cjw.
*/

struct FS_ObjLock *_fs_Lock_New_From_Node(
	struct FS_Struct *fs,
	struct FS_ObjNode *objnode,
	int32 *res2,
	int32 access )
{
struct FS_ObjLock *retval;
struct FS_ObjLock *lock;
struct FS_ObjLock *ic1;
U32 dostype;

	MYINFO( "PTP-FS : _fs_Lock_New_From_Node" );

	retval = NULL;

	if ( ! objnode )
	{
		MYERROR( "PTP-FS : NULL Pointer" );
		goto bailout;
	}

	/*
	**  Directories are always SHARED, regardless of request.
	*/

	if ( objnode->on_Type == FSO_TYPE_DIRECTORY )
	{
		access = SHARED_LOCK;
	}
	else
	{
		/*
		**  Search for exclusive locks on the same file,
		**  or any locks, if we want an exclusive lock ourself.
		*/

		for( ic1 = BADDR(fs->fs_LockList) ; ic1 ; ic1 = BADDR(ic1->Lock.fl_Link) )
		{
			if ( ic1->Node == objnode )
			{
				if ( access == EXCLUSIVE_LOCK )
				{
					/* we want exclusive access, but someone else has a lock of some kind */
					break;
				}

				if ( ic1->Lock.fl_Access == EXCLUSIVE_LOCK )
				{
					/* we want access, might even be shared, but someone else has an exclusive lock */
					break;
				}
			}
		}

		if ( ic1 )
		{
			/* We can't satisfy their request. */
			if ( res2 )
			{
				*res2 = ERROR_OBJECT_IN_USE;
			}
			goto bailout;
		}
	}

	/*
	** Get the dostype from the volume node if it's there.
	*/

	if ( fs->fs_VolumeNode )
	{
		dostype = fs->fs_VolumeNode->vn_DiskType;
	}
	else
	{
		dostype = ID_VP255_DOS_DISK;
	}

	/* it's always MEMF_CLEAR */
	lock = AllocDosObjectTags( DOS_LOCK,
		ADO_DOSType, dostype,
		ADO_Size, sizeof( struct FS_ObjLock ),
		TAG_END
	);

	if ( ! lock )
	{
		if ( res2 )
		{
			*res2 = ERROR_NO_FREE_STORE;
		}
		goto bailout;
	}

	/*
	** This next line is to keep those happy who use fl_Key as a Unix inode.
	*/

	lock->Node				= objnode;
	lock->Lock.fl_Key		= (S32) objnode;
	lock->Lock.fl_Access	= access;
	lock->Lock.fl_Port		= & fs->fs_FSVector_Port->MP;
	lock->Lock.fl_Volume	= MKBADDR( fs->fs_VolumeNode );

	/*
	** So others don't have to do this all over the place.
	** NOTE: for a file, current_block CANNOT be null any more.
	** May also be nodelist.
	*/
	
	lock->Current_Block = (PTR) GetHead( & objnode->on_Content_List );

	/*
	** For application compatibility and for obvious speed reasons,
	** we always put the new lock at the head of the locklist,
	** we do this by simply adding the existing locklist
	** to follow our new lock using BCPL linkage.
	*/

	/* add list to our lock */
	lock->Lock.fl_Link = fs->fs_LockList;

	/* put our lock as the locklist head. */
	fs->fs_LockList = MKBADDR(lock);		

	retval = lock;

	if ( res2 )
	{
		*res2 = 0;
	}

bailout:

	return( retval );
}

// --

