
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
_fs_Node_Online( struct FS_ObjNode *node )
{
	struct CameraInfoNode *camera;

	S32 valid;

	if ( ! node->on_CameraRoot )
	{
		return ( TRUE );
	}

	camera = node->on_CameraRoot->on_Camera;

	if ( ( ! camera ) || ( ( node->on_StorageRoot ) && ( node->on_StorageRoot->on_StorageRemoved ) ) )
	{
		return ( FALSE );
	}

	ObtainSemaphore( &camera->cin_Gate );

	valid = camera->cin_Online;

	if ( ( valid ) && ( node->on_StorageRoot ) )
	{
		valid = ( ! camera->cin_InvalidateStores ) &&
			( ! _fs_Store_Removed( node->on_Storage, camera->cin_Removals, camera->cin_RemovalCount ) );
	}

	ReleaseSemaphore( &camera->cin_Gate );

	return ( valid );
}

// --
