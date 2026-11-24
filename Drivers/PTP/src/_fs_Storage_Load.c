
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static U32
StorageTime( void )
{
	struct DateStamp now;

	DateStamp( &now );

	return ( (U32)now.ds_Days * 86400U + (U32)now.ds_Minute * 60U + (U32)now.ds_Tick / 50U );
}

// --

static void
RetireStore( struct FS_Struct *fs, struct FS_ObjNode *child )
{
	Remove( &child->on_Node );

	child->on_StorageRemoved = TRUE;

	if ( ! child->on_References )
	{
		_fs_Node_Delete( fs, child );
	}
}

// --

S32
_fs_Store_Removed( U32 storage, const U32 *removals, U32 count )
{
	U32 index;
	U32 id;

	for ( index = 0 ; index < count ; index++ )
	{
		id = removals[index];

		if ( ( id == storage ) || ( ! id ) || ( id == 0xffffffffU ) ||
			( ( ( id & 0xffffU ) == 0xffffU ) && ( ( id >> 16 ) == ( storage >> 16 ) ) ) )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

S32
_fs_Storage_Load( struct FS_Struct *fs, struct FS_ObjNode *node )
{
	struct CameraInfoNode *camera;
	struct FS_ObjNode *child;
	struct FS_ObjNode *next;
	struct CommandMessage request;

	U32 removals[PTP_MAX_REMOVALS];
	U32 removal_count;
	U32 generation;
	U32 count;
	U32 index;
	U32 id;
	U32 now;
	S32 invalidate;
	S32 reliable;
	S32 error;
	S32 available;
	STR name;

	camera = node->on_Camera;
	available = FALSE;

	if ( ! camera )
	{
		return ( ERROR_DEVICE_NOT_MOUNTED );
	}

	now = StorageTime();

	ObtainSemaphore( &camera->cin_Gate );

	generation = camera->cin_StorageGeneration;
	reliable = camera->cin_EventsReliable;
	invalidate = camera->cin_InvalidateStores;
	removal_count = camera->cin_RemovalCount;

	memcpy( removals, camera->cin_Removals, removal_count * sizeof( removals[0] ) );

	ReleaseSemaphore( &camera->cin_Gate );

	if ( ( node->on_Loaded ) && ( node->on_StorageGeneration == generation ) &&
		( ( reliable ) || ( now - node->on_StorageChecked < 2 ) ) )
	{
		return ( 0 );
	}

	// Retire known removals even if the subsequent USB lookup fails.
	for ( child = (PTR)GetHead( &node->on_Content_List ) ; child ; child = next )
	{
		next = (PTR)GetSucc( &child->on_Node );

		if ( ( invalidate ) || ( _fs_Store_Removed( child->on_Storage, removals, removal_count ) ) )
		{
			RetireStore( fs, child );
		}
		else
		{
			child->on_StorageSeen = FALSE;
		}
	}

	memset( &request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Get_Storage_IDs;
	request.cm_Allocate = TRUE;
	request.cm_WantData = TRUE;
	request.cm_Limit = PTP_METADATA_LIMIT;

	error = _fs_Camera_Request( camera, &request );

	if ( error )
	{
		goto done;
	}

	if ( request.cm_Actual < sizeof( count ) )
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto done;
	}

	memcpy( &count, request.cm_Data, sizeof( count ) );

	count = LE_SWAP32( count );

	if ( count > ( request.cm_Actual - sizeof( count ) ) / sizeof( id ) )
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto done;
	}

	for ( index = 0 ; index < count ; index++ )
	{
		memcpy( &id, request.cm_Data + sizeof( count ) + index * sizeof( id ), sizeof( id ) );

		id = LE_SWAP32( id );

		if ( ( ! id ) || ( id == 0xffffffffU ) )
		{
			continue;
		}

		for ( child = (PTR)GetHead( &node->on_Content_List ) ; child ; child = (PTR)GetSucc( &child->on_Node ) )
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

			child = _fs_Node_CreateDir( fs, &error, name );

			FreeVec( name );

			if ( ! child )
			{
				goto done;
			}

			child->on_Parent_dir = node;
			child->on_CameraRoot = node;
			child->on_StorageRoot = child;
			child->on_Storage = id;
			child->on_StorageNew = TRUE;

			// Keep staged names unique under the filesystem semaphore.
			AddTail( &node->on_Content_List, &child->on_Node );
		}

		child->on_StorageSeen = TRUE;

		if ( ( ! camera->cin_Apple ) || ( id != 0xfeedfaceU ) )
		{
			available = TRUE;
		}
	}

	ObtainSemaphore( &camera->cin_Gate );

	if ( ( ! camera->cin_Online ) || ( generation != camera->cin_StorageGeneration ) )
	{
		error = ERROR_DEVICE_NOT_MOUNTED;
	}
	else
	{
		node->on_StorageGeneration = generation;
		node->on_StorageChecked = now;
		node->on_Loaded = ( available ) || ( ! camera->cin_Apple );
		camera->cin_RemovalCount = 0;
		camera->cin_InvalidateStores = FALSE;
	}

	ReleaseSemaphore( &camera->cin_Gate );

done:

	for ( child = (PTR)GetHead( &node->on_Content_List ) ; child ; child = next )
	{
		next = (PTR)GetSucc( &child->on_Node );

		if ( ( ( error ) && ( child->on_StorageNew ) ) || ( ( ! error ) && ( ! child->on_StorageSeen ) ) )
		{
			RetireStore( fs, child );
		}
		else
		{
			child->on_StorageNew = FALSE;
		}
	}

	if ( error )
	{
		node->on_Loaded = FALSE;
	}

	FreeVec( request.cm_Data );

	return ( error );
}

// --
