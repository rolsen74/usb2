
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

int32 _fs__FSExamineDir( struct FSVP *vp, int32 *res2, struct PRIVATE_ExamineDirContext *ctx )
{
struct FS_Struct *fs;
struct ExamineData *exd;
struct FS_ObjNode *dirnode;
struct FS_ObjNode *entry;
struct FS_ObjLock *lock;
struct FS_ObjNode *next;
STR linktargetname;
TEXT slbuf[MAX_VP_SOFTLINK_TARGET+4];
U32 mask;

	MYINFO( "PTP-FS : _fs__FSExamineDir" );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( ! ctx )
	{
		MYERROR( "PTP-FS : NULL Pointer" );

		if ( res2 )
		{
			*res2 = ERROR_REQUIRED_ARG_MISSING;
		}
		goto bailout;
	}

	lock = (PTR) ctx->ReferenceLock;
	mask = ctx->DataRequestFlags;

	/*
	**  We generally do not bother trying to reuse nodes anymore,
	**  due to the high availability of RAM on modern machines,
	**  so just dump any stale nodes that are still on the stale list.
	*/

	while( TRUE )
	{
		exd = (PTR) RemHead( (PTR) & ctx->StaleNodeList );

		if ( ! exd )
		{
			break;
		}

		FreeDosObject( DOS_EXAMINEDATA, exd );
	}

	if ( lock )
	{
		/* get the directory node from the lock. */
		dirnode = lock->Node;
	}
	else
	{
		dirnode = NULL;
	}

	if ( ! dirnode )
	{
		MYERROR( "PTP-FS : NULL Pointer" );
		goto bailout;
	}

	/* we can't scan with anything but a dir */
	if ( dirnode->on_Type != FSO_TYPE_DIRECTORY )
	{
		if ( res2 )
		{
			*res2 = ERROR_OBJECT_WRONG_TYPE;
		}
		goto bailout;
	}

	for( entry = (PTR) GetHead( & dirnode->on_Content_List ) ; entry ; entry = next )
	{
		next = (PTR) GetSucc( & entry->on_Node );

		linktargetname = NULL;

		/*
		**  Don't get link names unless asked for them.
		*/

		if ( mask & EXF_LINK )
		{
			/**/ if (( FSOF_LINK|FSO_TYPE_FILE ) == entry->on_Type )
			{
				linktargetname = entry->on_HardLink_Target->on_Name;
			}
			else if (( FSOF_LINK|FSO_TYPE_DIRECTORY ) == entry->on_Type )
			{
				linktargetname = entry->on_HardLink_Target->on_Name;
			}
			else if (( FSOF_LINK|FSO_TYPE_SOFTLINK ) == entry->on_Type )
			{
				#warning fix me
//				  if ( read_softlink_target( fs, res2, entry, slbuf, sizeof(slbuf)) >= 0 )
				if ( 0 )
				{
					linktargetname = slbuf;
				}
				else
				{
					break;   /* result 2 set by read_softlink_target() */
				}
			}
		}

		#define safe_strlen(str)  ((str) ? strlen(str) : 0 )
		exd = AllocDosObjectTags( DOS_EXAMINEDATA,
			ADO_ExamineData_LinkSize,	1 + safe_strlen( linktargetname),
			ADO_ExamineData_NameSize,	1 + safe_strlen( entry->on_Name ),
			ADO_ExamineData_CommentSize,1 + safe_strlen( entry->on_Comment ),
			ADO_ExamineDir_Context,		ctx,
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

		if ( mask & EXF_TYPE )
		{
			exd->Type = entry->on_Type;
		}

		if ( mask & EXF_SIZE )
		{
			switch( entry->on_Type )
			{
				case FSO_TYPE_FILE:
				{
					exd->FileSize = (int64) entry->on_File_Size;
					break;
				}

				case (FSO_TYPE_FILE|FSOF_LINK):
				{
					exd->FileSize = (int64) entry->on_HardLink_Target->on_File_Size;
					break;
				}

				default:
				{
					exd->FileSize = -1LL;
					break;
				}
			}
		}

		if ( mask & EXF_DATE )
		{
			exd->Date.ds_Days   = entry->on_Date.ds_Days;
			exd->Date.ds_Minute = entry->on_Date.ds_Minute;
			exd->Date.ds_Tick   = entry->on_Date.ds_Tick;
		}

		if ( mask & EXF_REFCOUNT )
		{
			exd->RefCount = 0;				/* FIX: do something with this. */
		}

		if ( mask & EXF_OBJECTID )
		{
			exd->ObjectID = (U32) entry;	/* use inode address */
		}

		if ( mask & EXF_NAME )
		{
			if ( entry->on_Name )
			{
				strncpy( exd->Name, entry->on_Name, exd->NameSize );
			}
			else
			{
				exd->Name[0] = 0;
			}
		}

		if ( mask & EXF_COMMENT )
		{
			if ( entry->on_Comment )
			{
				strncpy( exd->Comment, entry->on_Comment, exd->CommentSize );
			}
			else
			{
				exd->Comment[0] = 0;
			}
		}

		if ( mask & EXF_LINK )
		{
			if ( linktargetname )
			{
				strncpy( exd->Link, linktargetname, exd->LinkSize );
			}
			else
			{
				exd->Link[0] = 0;
			}
		}

		if ( mask & EXF_PROTECTION )
		{
			exd->Protection = entry->on_Protection;
		}

		if ( mask & EXF_OWNER )
		{
			exd->OwnerUID = entry->on_Owner;
			exd->OwnerGID = entry->on_Group;
		}

		AddTail( (PTR) & ctx->FreshNodeList, (PTR) & exd->EXDnode );
	}

	if ( ! entry )
	{
		if ( res2 )
		{
			/* actually this means success for this case. */
			*res2 = ERROR_NO_MORE_ENTRIES;
		}
	}

bailout:

	ReleaseSemaphore( & fs->fs_Semaphore );

	/*
	** We either get the whole dir in one go,
	** or we fail with a res2 code set.
	** DOS only saves the res2 code when
	** a primary FALSE returncode is set.
	*/

	return(	0 );
}

// --

