
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
Read32( const U8 *data )
{
	return ( (U32) data[0] | ( (U32) data[1] << 8 ) | ( (U32) data[2] << 16 ) | ( (U32) data[3] << 24 ) );
}

// --

// This index exists only while a directory is being loaded under fs_Semaphore.
struct NameIndex
{
	struct FS_ObjNode **Buckets;
	U32 Mask;
};

// --

static U32
NameBucket( const struct NameIndex *names, STR name, U32 length )
{
	U32 hash;
	U32 index;

	hash = 2166136261U;

	for ( index = 0 ; index < length ; index++ )
	{
		hash = ( hash ^ (U8)ToUpper( (U8)name[index] ) ) * 16777619U;
	}

	return ( hash & names->Mask );
}

// --

static void
IndexName( struct NameIndex *names, struct FS_ObjNode *child )
{
	U32 bucket;

	if ( names->Buckets )
	{
		bucket = NameBucket( names, child->on_Name, child->on_NameLen );

		child->on_NameIndexNext = names->Buckets[bucket];
		names->Buckets[bucket] = child;
	}
}

// --

static struct FS_ObjNode *
FindIndexedName( struct NameIndex *names, struct FS_ObjNode *parent, STR name, U32 length )
{
	struct FS_ObjNode *child;

	if ( ! names->Buckets )
	{
		return ( _fs_Node_Find( parent, name, length ) );
	}

	for ( child = names->Buckets[NameBucket( names, name, length )] ; child ; child = child->on_NameIndexNext )
	{
		if ( ( child->on_NameLen == length ) && ( ! Strnicmp( child->on_Name, name, length ) ) )
		{
			return ( child );
		}
	}

	return ( NULL );
}

// --

static S32
AddObject( struct FS_Struct *fs, struct FS_ObjNode *parent, struct NameIndex *names, U32 id )
{
	struct FS_ObjNode *node;
	struct FS_ObjNode *ancestor;
	struct CommandMessage request;

	U8 *data;
	U32 index;
	U32 length;
	U32 code;
	U32 size;
	U32 suffix;
	U16 format;
	S32 error;
	char name[256];
	STR unique;

	for( ancestor = parent; ancestor; ancestor = ancestor->on_Parent_dir )
	{
		if ( ancestor->on_Handle == id )
		{
			return ( 0 );
		}
	}

	memset( & request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Get_Object_Info;
	request.cm_ParameterCount = 1;
	request.cm_Parameters[0] = id;
	request.cm_Allocate = TRUE;
	request.cm_WantData = TRUE;
	request.cm_Limit = PTP_METADATA_LIMIT;

	error = _fs_Camera_Request_Node( parent, &request );

	if ( error )
	{
		FreeVec( request.cm_Data );

		return ( request.cm_Response == PTP_Response_InvalidHandle ? 0 : error );
	}

	data = request.cm_Data;

	if ( request.cm_Actual < 53 )
	{
		FreeVec( data );

		return ( ERROR_OBJECT_WRONG_TYPE );
	}

	length = data[52];
	size = Read32( data + 8 );
	format = data[4] | ( (U16) data[5] << 8 );

	if ( ( 53 + length * 2 > request.cm_Actual ) || ( length && ( data[51 + length * 2] || data[52 + length * 2] ) ) )
	{
		FreeVec( data );

		return ( ERROR_OBJECT_WRONG_TYPE );
	}

	for( index = 0; index + 1 < length; index++ )
	{
		code = data[53 + index * 2] | ( (U32) data[54 + index * 2] << 8 );

		name[index] = ( ( code < 32 ) || ( code >= 255 ) || ( code == '/' ) || ( code == ':' ) ) ? '_' : code;
	}

	name[index] = 0;

	FreeVec( data );

	unique = NULL;
	node = index ? FindIndexedName( names, parent, name, index ) : NULL;

	if ( ( node ) && ( node->on_Handle == id ) )
	{
		return ( 0 );
	}

	if ( ( ! index ) || ( node ) )
	{
		// Reserve room for the handle and keep the camera filename within the DOS limit.
		name[235] = 0;
		suffix = 0;

		for( ;; )
		{
			if ( suffix )
			{
				unique = ASPrintf( "%08lx-%08lx-%s", id, suffix, index ? name : "Object" );
			}
			else
			{
				unique = ASPrintf( "%08lx-%s", id, index ? name : "Object" );
			}

			if ( ! unique )
			{
				return ( ERROR_NO_FREE_STORE );
			}

			node = FindIndexedName( names, parent, unique, strlen( unique ) );

			if ( ! node )
			{
				break;
			}

			FreeVec( unique );

			if ( node->on_Handle == id )
			{
				return ( 0 );
			}

			suffix++;
		}
	}

	node = _fs_Node_CreateDir( fs, & error, unique ? unique : name );

	FreeVec( unique );

	if ( ! node )
	{
		return ( error );
	}

	node->on_Parent_dir = parent;
	node->on_CameraRoot = parent->on_CameraRoot;
	node->on_StorageRoot = parent->on_StorageRoot;
	node->on_Storage = parent->on_Storage;
	node->on_Handle = id;
	node->on_Depth = parent->on_Depth + 1;

	if ( format != 0x3001 )
	{
		node->on_Type = FSO_TYPE_FILE;
		node->on_File_Size = size;
	}

	AddTail( &parent->on_Content_List, &node->on_Node );

	IndexName( names, node );

	return ( 0 );
}

// --

S32
_fs_Node_Load( struct FS_Struct *fs, struct FS_ObjNode *node )
{
	struct FS_ObjNode *child;
	struct CommandMessage request;
	struct NameIndex names;

	U32 count;
	U32 index;
	U32 id;
	U32 buckets;
	S32 error;

	memset( &names, 0, sizeof( names ) );

	if ( ! node->on_CameraRoot )
	{
		return ( 0 );
	}

	if ( ! _fs_Node_Online( node ) )
	{
		return ( ERROR_DEVICE_NOT_MOUNTED );
	}

	if ( node == node->on_CameraRoot )
	{
		return ( _fs_Storage_Load( fs, node ) );
	}

	if ( node->on_Loaded )
	{
		return ( 0 );
	}

	if ( node->on_Depth >= PTP_MAX_DEPTH )
	{
		return ( ERROR_OBJECT_WRONG_TYPE );
	}

	memset( & request, 0, sizeof( request ) );

	request.cm_Allocate = TRUE;
	request.cm_WantData = TRUE;
	request.cm_Limit = PTP_METADATA_LIMIT;

	request.cm_Operation = PTP_Get_Object_Handles;
	request.cm_ParameterCount = 3;
	request.cm_Parameters[0] = node->on_Storage;
	request.cm_Parameters[2] = node->on_Handle ? node->on_Handle : 0xffffffffU;

	error = _fs_Camera_Request_Node( node, &request );

	if ( error )
	{
		goto done;
	}

	if ( request.cm_Actual < 4 )
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto done;
	}

	count = Read32( request.cm_Data );

	if ( count > ( request.cm_Actual - 4 ) / 4 )
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto done;
	}

	if ( count > 16 )
	{
		for ( buckets = 16 ; ( buckets < count ) && ( buckets < 4096 ) ; buckets <<= 1 )
		{
		}

		names.Mask = buckets - 1;

		names.Buckets = AllocVecTags( buckets * sizeof( names.Buckets[0] ), AVT_Clear, 0, TAG_END );

		for ( child = (PTR)GetHead( &node->on_Content_List ) ; child ; child = (PTR)GetSucc( &child->on_Node ) )
		{
			IndexName( &names, child );
		}
	}

	for ( index = 0 ; index < count ; index++ )
	{
		id = Read32( request.cm_Data + 4 + index * 4 );

		if ( ( ! id ) || ( id == 0xffffffffU ) )
		{
			continue;
		}

		error = AddObject( fs, node, &names, id );

		if ( error )
		{
			break;
		}
	}

	if ( error )
	{
		while(( child = (PTR) RemHead( & node->on_Content_List ) ))
		{
			_fs_Node_Delete( fs, child );
		}
	}
	else
	{
		node->on_Loaded = TRUE;
	}

done:

	FreeVec( names.Buckets );

	FreeVec( request.cm_Data );

	return ( error );
}

// --
