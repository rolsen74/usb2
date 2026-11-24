
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

LONG
_fs__FSRead( struct FSVP *vp, LONG *res2, struct FileHandle *file, STR buffer, LONG count )
{
	struct FS_Struct *fs;
	struct FS_ObjLock *lock;
	struct FS_ObjNode *node;
	struct CameraInfoNode *camera;
	struct CommandMessage request;

	S32 result;
	S32 error;

	fs = vp->FSV.FSPrivate;
	result = -1;
	error = 0;

	ObtainSemaphore( & fs->fs_Semaphore );

	if ( ( ! file ) || ( ! file->fh_Arg2 ) || ( count < 0 ) || ( ( count ) && ( ! buffer ) ) )
	{
		error = ERROR_REQUIRED_ARG_MISSING;
		goto done;
	}

	lock = file->fh_Arg2;
	node = lock->Node;
	camera = node->on_CameraRoot->on_Camera;

	if ( ! _fs_Node_Online( node ) )
	{
		error = ERROR_DEVICE_NOT_MOUNTED;
		goto done;
	}

	error = _fs_Storage_Load( fs, node->on_CameraRoot );

	if ( error )
	{
		goto done;
	}

	if ( ! _fs_Node_Online( node ) )
	{
		error = ERROR_DEVICE_NOT_MOUNTED;
		goto done;
	}

	if ( ( ! count ) || ( lock->Position >= (U64) node->on_File_Size ) )
	{
		result = 0;
		goto done;
	}

	if ( node->on_File_Size == 0xffffffffU )
	{
		error = ERROR_OBJECT_TOO_LARGE;
		goto done;
	}

	count = MIN( (U64) count, (U64) node->on_File_Size - lock->Position );

	if ( ( ! node->on_Pending ) && ( ! lock->Cache ) && ( ! camera->cin_NoPartial ) )
	{
		memset( & request, 0, sizeof( request ) );

		request.cm_Operation = PTP_Get_Partial_Object;
		request.cm_ParameterCount = 3;
		request.cm_Parameters[0] = node->on_Handle;
		request.cm_Parameters[1] = lock->Position;
		request.cm_Parameters[2] = count;
		request.cm_WantData = TRUE;
		request.cm_Data = (PTR) buffer;
		request.cm_Limit = count;

		error = _fs_Camera_Request_Node( node, &request );

		if ( ! error )
		{
			result = request.cm_Actual;

			if ( ! result )
			{
				error = ERROR_SEEK_ERROR;
			}
		}
		else if ( request.cm_Response == PTP_Response_Unsupported )
		{
			camera->cin_NoPartial = TRUE;
			error = 0;
		}
	}

	if ( ( ! error ) && ( camera->cin_NoPartial ) && ( ! lock->Cache ) )
	{
		error = _fs_FileCache( lock );
	}

	if ( ( ! error ) && ( lock->Cache ) )
	{
		if ( ! ChangeFilePosition( lock->Cache, lock->Position, OFFSET_BEGINNING ) )
		{
			error = IoErr();

			if ( ! error )
			{
				error = ERROR_SEEK_ERROR;
			}
		}
		else
		{
			result = Read( lock->Cache, buffer, count );

			if ( result < 0 )
			{
				error = IoErr();

				if ( ! error )
				{
					error = ERROR_SEEK_ERROR;
				}
			}
		}
	}

	if ( ( ! error ) && ( result >= 0 ) )
	{
		if ( lock->Cache )
		{
			error = _fs_Camera_Fence( node );
		}

		if ( ! error )
		{
			lock->Position += result;
		}
	}

done:

	ReleaseSemaphore( & fs->fs_Semaphore );

	if ( res2 )
	{
		*res2 = error;
	}

	return ( error ? -1 : result );
}

// --
