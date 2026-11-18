
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

STR
_fs_Storage_Name( struct FS_ObjNode *parent, U32 id )
{
	struct CameraInfoNode *camera;
	struct CommandMessage request;

	const U8 *data;
	U32 remaining;
	U32 bytes;
	S32 error;
	STR base;
	char description[127];
	char label[127];

	camera = parent->on_Camera;
	description[0] = 0;
	label[0] = 0;

	if ( camera->cin_CanReadStorageInfo )
	{
		memset( & request, 0, sizeof( request ) );

		request.cm_Operation = PTP_Get_Storage_Info;
		request.cm_ParameterCount = 1;
		request.cm_Parameters[0] = id;
		request.cm_Allocate = TRUE;
		request.cm_WantData = TRUE;
		// Fixed fields followed by two PTP strings of up to 255 UTF-16 units each.
		request.cm_Limit = PTP_StorageInfo_Strings + 2 * ( 1 + 2 * 255 );

		error = _fs_Camera_Request( camera, & request );

		if ( ( ! error ) && ( request.cm_Actual > PTP_StorageInfo_Strings ) )
		{
			data = request.cm_Data + PTP_StorageInfo_Strings;
			remaining = request.cm_Actual - PTP_StorageInfo_Strings;

			bytes = _ptp_Read_Name( data, remaining, description, sizeof( description ) );

			if ( bytes )
			{
				_ptp_Read_Name( data + bytes, remaining - bytes, label, sizeof( label ) );
			}
		}

		FreeVec( request.cm_Data );
	}

	if ( label[0] )
	{
		base = ASPrintf( "%s", label );
	}
	else if ( description[0] )
	{
		base = ASPrintf( "%s", description );
	}
	else
	{
		base = ASPrintf( "Storage-%08lx", id );
	}

	return ( _fs_Unique_Name( parent, base ) );
}

// --
