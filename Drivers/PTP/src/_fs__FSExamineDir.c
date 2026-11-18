
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

LONG
_fs__FSExamineDir( struct FSVP *vp, LONG *res2, struct PRIVATE_ExamineDirContext *context )
{
	struct FS_Struct *fs;
	struct FS_ObjLock *lock;
	struct FS_ObjNode *directory;
	struct FS_ObjNode *entry;
	struct ExamineData *data;

	S32 error;

	if ( ! context )
	{
		if ( res2 )
		{
			*res2 = ERROR_REQUIRED_ARG_MISSING;
		}

		return ( DOSFALSE );
	}

	fs = vp->FSV.FSPrivate;
	lock = (PTR) context->ReferenceLock;

	ObtainSemaphore( & fs->fs_Semaphore );

	while(( data = (PTR) RemHead( (PTR) & context->StaleNodeList ) ))
	{
		FreeDosObject( DOS_EXAMINEDATA, data );
	}

	if ( context->FSPrivate[0] )
	{
		error = context->FSPrivate[0];
		goto done;
	}

	directory = lock ? lock->Node : fs->fs_RootNode;

	if ( directory->on_Type != FSO_TYPE_DIRECTORY )
	{
		error = ERROR_OBJECT_WRONG_TYPE;
	}
	else
	{
		error = _fs_Node_Load( fs, directory );
	}

	if ( ! error )
	{
		for( entry = (PTR) GetHead( & directory->on_Content_List ); entry;
			entry = (PTR) GetSucc( & entry->on_Node ) )
		{
			if ( entry->on_Pending )
			{
				continue;
			}

			data = _fs_ExamineNode( entry, context );

			if ( ! data )
			{
				error = ERROR_NO_FREE_STORE;
				break;
			}

			AddTail( (PTR) & context->FreshNodeList, (PTR) & data->EXDnode );
		}
	}

	if ( error )
	{
		// An aborted scan must not start over if DOS asks for a refill.
		context->FSPrivate[0] = error;

		while(( data = (PTR) RemHead( (PTR) & context->FreshNodeList ) ))
		{
			FreeDosObject( DOS_EXAMINEDATA, data );
		}
	}

done:

	ReleaseSemaphore( & fs->fs_Semaphore );

	if ( res2 )
	{
		// DOS consumes the fresh list before reporting that the scan is complete.
		*res2 = error ? error : ERROR_NO_MORE_ENTRIES;
	}

	return ( DOSFALSE );
}

// --
