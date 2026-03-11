
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct ExamineData *_fs__FSExamineLock( struct FSVP *vp, int32 *res2, struct Lock *objLock )
{
struct FS_Struct *fs;
struct ExamineData *exd;
struct FS_ObjNode *objnode;
struct FS_ObjLock *lock;

	MYINFO( "PTP-FS : _fs__FSExamineLock" );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );


	exd = NULL;

	lock = (PTR) objLock;

	if ( ! lock )
	{
		MYERROR( "PTP-FS : Mising Lock" );

		if ( res2 )
		{
			*res2 = ERROR_REQUIRED_ARG_MISSING;
		}
		goto bailout;
	}

	objnode = lock->Node;

	if ( ! objnode )
	{
		MYERROR( "PTP-FS : NULL Pointer" );
		goto bailout;
	}

	/*
	**  If the node is for a hardlink,
	**  show info for the target instead.
	*/

	/**/ if (( FSOF_LINK|FSO_TYPE_FILE ) == objnode->on_Type )
	{
		objnode = objnode->on_HardLink_Target;
	}
	else if (( FSOF_LINK|FSO_TYPE_DIRECTORY ) == objnode->on_Type )
	{
		objnode = objnode->on_HardLink_Target;
	}

	if ( ! objnode )
	{
		MYERROR( "PTP-FS : NULL Pointer" );
		goto bailout;
	}

	#define safe_strlen(str)  ((str) ? strlen(str) : 0 )
	exd = AllocDosObjectTags( DOS_EXAMINEDATA,
		ADO_ExamineData_NameSize,	1 + safe_strlen( objnode->on_Name ),
		ADO_ExamineData_CommentSize,1 + safe_strlen( objnode->on_Comment ),  /* can be NULL */
		TAG_END
	);

	if ( ! exd )
	{
		if ( res2 )
		{
			*res2 = ERROR_NO_FREE_STORE;
		}
		goto bailout;
	}

	exd->Type = objnode->on_Type;

	switch( exd->Type )
	{
		case FSO_TYPE_FILE:
		{
			exd->FileSize = (int64) objnode->on_File_Size;
			break;
		}

		case (FSO_TYPE_FILE|FSOF_LINK):
		{
			exd->FileSize = (int64) objnode->on_HardLink_Target->on_File_Size;
			break;
		}

		default:
		{
			exd->FileSize = -1LL;
			break;
		}
	}

	strncpy( exd->Name, objnode->on_Name, exd->NameSize );

	if ( objnode->on_Comment )
	{
		strncpy( exd->Comment, objnode->on_Comment, exd->CommentSize );
	}

	exd->Date.ds_Days	= objnode->on_Date.ds_Days;
	exd->Date.ds_Minute	= objnode->on_Date.ds_Minute;
	exd->Date.ds_Tick	= objnode->on_Date.ds_Tick;

	exd->RefCount		= 0;					/* FIX: do something with this. */
	exd->ObjectID		= (U32) objnode;		/* use inode address */
	exd->Protection		= objnode->on_Protection;
	exd->OwnerUID		= objnode->on_Owner;
	exd->OwnerGID		= objnode->on_Group;

bailout:

	ReleaseSemaphore( & fs->fs_Semaphore );

	return(	exd );
}

// --

