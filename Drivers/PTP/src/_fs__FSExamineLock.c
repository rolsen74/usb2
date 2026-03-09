
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
struct PTP_FSStruct *fs;
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

	/**/ if (( FSOF_LINK|FSO_TYPE_FILE ) == objnode->Type )
	{
		objnode = objnode->HardLink_Target;
	}
	else if (( FSOF_LINK|FSO_TYPE_DIRECTORY ) == objnode->Type )
	{
		objnode = objnode->HardLink_Target;
	}

	if ( ! objnode )
	{
		MYERROR( "PTP-FS : NULL Pointer" );
		goto bailout;
	}

	#define safe_strlen(str)  ((str) ? strlen(str) : 0 )
	exd = AllocDosObjectTags( DOS_EXAMINEDATA,
		ADO_ExamineData_NameSize,	1 + safe_strlen( objnode->Name ),
		ADO_ExamineData_CommentSize,1 + safe_strlen( objnode->Comment ),  /* can be NULL */
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

	exd->Type = objnode->Type;

	switch( exd->Type )
	{
		case FSO_TYPE_FILE:
		{
			exd->FileSize = (int64) objnode->File_Size;
			break;
		}

		case (FSO_TYPE_FILE|FSOF_LINK):
		{
			exd->FileSize = (int64) objnode->HardLink_Target->File_Size;
			break;
		}

		default:
		{
			exd->FileSize = -1LL;
			break;
		}
	}

	strncpy( exd->Name, objnode->Name, exd->NameSize );

	if ( objnode->Comment )
	{
		strncpy( exd->Comment, objnode->Comment, exd->CommentSize );
	}

	exd->Date.ds_Days	= objnode->Date.ds_Days;
	exd->Date.ds_Minute	= objnode->Date.ds_Minute;
	exd->Date.ds_Tick	= objnode->Date.ds_Tick;

	exd->RefCount		= 0;					/* FIX: do something with this. */
	exd->ObjectID		= (U32) objnode;		/* use inode address */
	exd->Protection		= objnode->Protection;
	exd->OwnerUID		= objnode->Owner;
	exd->OwnerGID		= objnode->Group;

bailout:

	ReleaseSemaphore( & fs->fs_Semaphore );

	return(	exd );
}

// --

