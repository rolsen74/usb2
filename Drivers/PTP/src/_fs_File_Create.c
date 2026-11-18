
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct FS_ObjLock *
_fs_File_Create( struct FS_Struct *fs, S32 *res2, struct FS_ObjLock *rel, STR name )
{
	struct FS_ObjNode *parent;
	struct FS_ObjNode *node;
	struct FS_ObjLock *lock;
	struct CameraInfoNode *camera;
	struct CommandMessage request;

	U32 length;
	U32 index;
	U8 ch;

	if ( ( ! name ) || ( ! *name ) )
	{
		*res2 = ERROR_INVALID_COMPONENT_NAME;

		return ( NULL );
	}

	length = strlen( name );

	if ( ( length > 254 ) || ( length == 1 && name[0] == '*' ) )
	{
		*res2 = ERROR_INVALID_COMPONENT_NAME;

		return ( NULL );
	}

	for( index = 0; index < length; index++ )
	{
		ch = name[index];

		if ( ( ch < 32 ) || ( ch == 255 ) || ( ch == ':' ) || ( ch == '/' ) )
		{
			*res2 = ERROR_INVALID_COMPONENT_NAME;

			return ( NULL );
		}
	}

	parent = _fs_Node_Locate_Rel( fs, res2, rel, "" );

	if ( ! parent )
	{
		return ( NULL );
	}

	if ( parent->on_Type != FSO_TYPE_DIRECTORY )
	{
		*res2 = ERROR_OBJECT_WRONG_TYPE;

		return ( NULL );
	}

	camera = parent->on_CameraRoot ? parent->on_CameraRoot->on_Camera : NULL;

	if ( ( ! parent->on_Storage ) || ( ! camera ) || ( ! camera->cin_CanUpload ) )
	{
		*res2 = ERROR_DISK_WRITE_PROTECTED;

		return ( NULL );
	}

	*res2 = _fs_Node_Load( fs, parent );

	if ( *res2 )
	{
		return ( NULL );
	}

	node = _fs_Node_Find( parent, name, length );

	if ( node )
	{
		if ( node->on_Pending )
		{
			*res2 = ERROR_OBJECT_IN_USE;
		}
		else
		{
			*res2 = node->on_Type == FSO_TYPE_DIRECTORY ? ERROR_OBJECT_WRONG_TYPE : ERROR_WRITE_PROTECTED;
		}

		return ( NULL );
	}

	memset( & request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Get_Storage_Info;
	request.cm_ParameterCount = 1;
	request.cm_Parameters[0] = parent->on_Storage;
	request.cm_Allocate = TRUE;
	request.cm_WantData = TRUE;
	request.cm_Limit = PTP_METADATA_LIMIT;

	*res2 = _fs_Camera_Request( camera, & request );

	if ( ! *res2 )
	{
		if ( request.cm_Actual < PTP_StorageInfo_Strings )
		{
			*res2 = ERROR_OBJECT_WRONG_TYPE;
		}
		else if ( request.cm_Data[4] || request.cm_Data[5] )
		{
			*res2 = ERROR_DISK_WRITE_PROTECTED;
		}
	}

	FreeVec( request.cm_Data );

	if ( *res2 )
	{
		return ( NULL );
	}

	node = _fs_Node_CreateDir( fs, res2, name );

	if ( ! node )
	{
		return ( NULL );
	}

	node->on_Parent_dir = parent;
	node->on_CameraRoot = parent->on_CameraRoot;
	node->on_StorageRoot = parent->on_StorageRoot;
	node->on_Storage = parent->on_Storage;
	node->on_Depth = parent->on_Depth + 1;
	node->on_Type = FSO_TYPE_FILE;

	lock = _fs_Lock_New_From_Node( fs, node, res2, EXCLUSIVE_LOCK );

	if ( ! lock )
	{
		_fs_Node_Delete( fs, node );

		return ( NULL );
	}

	node->on_Pending = TRUE;

	AddTail( & parent->on_Content_List, & node->on_Node );

	*res2 = _fs_FileCache_Create( lock, MODE_READWRITE );

	if ( *res2 )
	{
		_fs_Lock_Free( fs, lock );

		return ( NULL );
	}

	return ( lock );
}

// --
