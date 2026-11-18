
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
_fs_Storage_Load( struct FS_Struct *fs, struct FS_ObjNode *node )
{
	struct CameraInfoNode *camera;
	struct FS_ObjNode *child;
	struct FS_ObjNode *next;
	struct CommandMessage request;

	U32 generation;
	U32 count;
	U32 index;
	U32 id;
	S32 error;
	S32 available;
	STR name;

	camera = node->on_Camera;
	available = FALSE;

	ObtainSemaphore( & camera->cin_Gate );

	generation = camera->cin_StorageGeneration;

	ReleaseSemaphore( & camera->cin_Gate );

	if ( ( node->on_Loaded ) && ( node->on_StorageGeneration == generation ) )
	{
		return ( 0 );
	}

	memset( & request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Get_Storage_IDs;
	request.cm_Allocate = TRUE;
	request.cm_WantData = TRUE;
	request.cm_Limit = PTP_METADATA_LIMIT;

	error = _fs_Camera_Request( camera, & request );

	if ( error )
	{
		goto done;
	}

	if ( request.cm_Actual < sizeof( count ) )
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto done;
	}

	memcpy( & count, request.cm_Data, sizeof( count ) );

	count = LE_SWAP32( count );

	if ( count > ( request.cm_Actual - sizeof( count ) ) / sizeof( id ) )
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto done;
	}

	for( child = (PTR) GetHead( & node->on_Content_List ); child; child = (PTR) GetSucc( & child->on_Node ) )
	{
		child->on_StorageSeen = FALSE;
	}

	for( index = 0; index < count; index++ )
	{
		memcpy( & id, request.cm_Data + sizeof( count ) + index * sizeof( id ), sizeof( id ) );

		id = LE_SWAP32( id );

		if ( ( ! id ) || ( id == 0xffffffffU ) )
		{
			continue;
		}

		for( child = (PTR) GetHead( & node->on_Content_List ); child; child = (PTR) GetSucc( & child->on_Node ) )
		{
			if ( child->on_Storage == id )
			{
				break;
			}
		}

		if ( ! child )
		{
			name = _fs_Storage_Name( node, id );

			if ( ! name )
			{
				error = ERROR_NO_FREE_STORE;
				goto done;
			}

			child = _fs_Node_CreateDir( fs, & error, name );

			FreeVec( name );

			if ( ! child )
			{
				goto done;
			}

			child->on_Parent_dir = node;
			child->on_CameraRoot = node;
			child->on_StorageRoot = child;
			child->on_Storage = id;

			AddTail( & node->on_Content_List, & child->on_Node );
		}
		child->on_StorageSeen = TRUE;

		if ( ( ! camera->cin_Apple ) || ( id != 0xfeedfaceU ) )
		{
			available = TRUE;
		}
	}

	for( child = (PTR) GetHead( & node->on_Content_List ); child; child = next )
	{
		next = (PTR) GetSucc( & child->on_Node );

		if ( ! child->on_StorageSeen )
		{
			Remove( & child->on_Node );

			child->on_StorageRemoved = TRUE;

			if ( ! child->on_References )
			{
				_fs_Node_Delete( fs, child );
			}
		}
	}

	node->on_StorageGeneration = generation;
	// Keep the empty iOS store visible and retry until photo storage is available.
	node->on_Loaded = ( available ) || ( ! camera->cin_Apple );

done:

	FreeVec( request.cm_Data );

	return ( error );
}

// --
