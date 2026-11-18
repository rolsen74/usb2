
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32
_fs_FileCache( struct FS_ObjLock *lock )
{
	struct FS_ObjNode *node;
	struct CommandMessage request;

	S32 error;

	node = lock->Node;

	error = _fs_FileCache_Create( lock, MODE_NEWFILE );

	if ( error )
	{
		return ( error );
	}

	memset( & request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Get_Object;
	request.cm_ParameterCount = 1;
	request.cm_Parameters[0] = node->on_Handle;
	request.cm_WantData = TRUE;
	request.cm_Limit = node->on_File_Size;
	request.cm_Output = lock->Cache;

	error = _fs_Camera_Request( node->on_CameraRoot->on_Camera, & request );

	if ( ( ! error ) && ( request.cm_Actual != node->on_File_Size ) )
	{
		error = ERROR_SEEK_ERROR;
	}

	if ( ! error )
	{
		if ( ! Close( lock->Cache ) )
		{
			error = IoErr();

			if ( ! error )
			{
				error = ERROR_DISK_FULL;
			}
		}

		lock->Cache = 0;

		if ( ! error )
		{
			lock->Cache = Open( lock->CacheName, MODE_OLDFILE );

			if ( ! lock->Cache )
			{
				error = IoErr();

				if ( ! error )
				{
					error = ERROR_OBJECT_NOT_FOUND;
				}
			}
		}
	}

	if ( error )
	{
		_fs_FileCache_Close( lock );
	}

	return ( error );
}

// --
