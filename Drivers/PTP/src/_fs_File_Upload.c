
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static U16
ObjectFormat( CONST_STRPTR name )
{
	static const struct
	{
		CONST_STRPTR Extension;
		U16 Format;
	} formats[] =
	{
		{ "jpg", 0x3801 }, { "jpeg", 0x3801 }, { "tif", 0x380d },
		{ "tiff", 0x380d }, { "bmp", 0x3804 }, { "gif", 0x3807 },
		{ "png", 0x380b }, { "txt", 0x3004 }, { "wav", 0x3008 },
		{ "mp3", 0x3009 }, { "avi", 0x300a }, { "mpg", 0x300b },
		{ "mpeg", 0x300b },
	};

	CONST_STRPTR extension;
	U32 length;
	U32 index;

	extension = strrchr( name, '.' );

	if ( extension )
	{
		extension++;

		length = strlen( extension );

		for( index = 0; index < sizeof( formats ) / sizeof( formats[0] ); index++ )
		{
			if ( ( strlen( formats[index].Extension ) == length ) &&
				( ! Strnicmp( extension, formats[index].Extension, length ) ) )
			{
				return ( formats[index].Format );
			}
		}
	}

	return ( 0x3000 );
}

// --

int32
_fs_File_Upload( struct FS_ObjLock *lock )
{
	struct FS_ObjNode *node;
	struct CameraInfoNode *camera;
	struct CommandMessage request;

	U8 info[52 + 1 + 2 * 255 + 3];
	U32 parent;
	U32 handle;
	U32 index;
	U32 size;
	U16 format;
	int32 error;
	S32 closed;

	if ( lock->WriteError )
	{
		return ( lock->WriteError );
	}

	node = lock->Node;
	camera = node->on_CameraRoot->on_Camera;

	if ( ! camera )
	{
		return ( ERROR_DEVICE_NOT_MOUNTED );
	}

	closed = Close( lock->Cache );

	lock->Cache = 0;

	if ( ! closed )
	{
		error = IoErr();

		return ( error ? error : ERROR_DISK_FULL );
	}

	lock->Cache = Open( lock->CacheName, MODE_OLDFILE );

	if ( ! lock->Cache )
	{
		error = IoErr();

		return ( error ? error : ERROR_OBJECT_NOT_FOUND );
	}

	parent = node->on_Parent_dir->on_Handle;
	size = node->on_File_Size;

	format = ObjectFormat( node->on_Name );

	memset( info, 0, sizeof( info ) );

	// The wire dataset is little endian with a counted UTF-16 filename.
	info[4] = format;
	info[5] = format >> 8;
	info[8] = size;
	info[9] = size >> 8;
	info[10] = size >> 16;
	info[11] = size >> 24;
	info[52] = node->on_NameLen + 1;

	for( index = 0; index < node->on_NameLen; index++ )
	{
		info[53 + 2 * index] = node->on_Name[index];
	}

	memset( & request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Send_Object_Info;
	request.cm_ParameterCount = 2;
	request.cm_Parameters[0] = node->on_Storage;
	request.cm_Parameters[1] = parent ? parent : 0xffffffffU;
	request.cm_SendData = TRUE;
	request.cm_Data = info;
	request.cm_Limit = 53 + 2 * ( node->on_NameLen + 1 ) + 3;

	error = _fs_Camera_Request( camera, & request );

	if ( error )
	{
		return ( error );
	}

	handle = request.cm_Reply[2];

	if ( ( request.cm_ReplyCount != 3 ) || ( request.cm_Reply[0] != node->on_Storage ) ||
		( ( request.cm_Reply[1] != parent ) && ( parent || request.cm_Reply[1] != 0xffffffffU ) ) ||
		( ! handle ) || ( handle == 0xffffffffU ) )
	{
		return ( ERROR_OBJECT_WRONG_TYPE );
	}

	memset( & request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Send_Object;
	request.cm_SendData = TRUE;
	request.cm_Input = lock->Cache;
	request.cm_Limit = size;

	error = _fs_Camera_Request( camera, & request );

	if ( ! error )
	{
		node->on_Handle = handle;
		node->on_Pending = FALSE;

		DateStamp( & node->on_Parent_dir->on_Date );
	}

	return ( error );
}

// --
