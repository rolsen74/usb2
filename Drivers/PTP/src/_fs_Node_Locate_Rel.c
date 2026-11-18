
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct FS_ObjNode *
_fs_Node_Locate_Rel( struct FS_Struct *fs, S32 *res2, struct FS_ObjLock *lock, STR name )
{
	struct FS_ObjNode *node;

	STR end;
	S32 error;

	node = lock ? lock->Node : fs->fs_RootNode;
	error = 0;

	if ( ! name )
	{
		name = "";
	}

	if ( ! Strnicmp( name, "PTP:", 4 ) )
	{
		name += 4;
		node = fs->fs_RootNode;
	}

	while( node )
	{
		if ( ( node->on_CameraRoot ) && ( ! node->on_CameraRoot->on_Camera ) )
		{
			error = ERROR_DEVICE_NOT_MOUNTED;
			break;
		}

		if ( ! *name )
		{
			break;
		}

		if ( node->on_Type != FSO_TYPE_DIRECTORY )
		{
			error = ERROR_OBJECT_WRONG_TYPE;
			break;
		}

		if ( *name == '/' )
		{
			node = node->on_Parent_dir;
			name++;
			continue;
		}

		error = _fs_Node_Load( fs, node );

		if ( error )
		{
			break;
		}

		end = strchr( name, '/' );

		if ( ! end )
		{
			end = name + strlen( name );
		}

		node = _fs_Node_Find( node, name, end - name );

		if ( ( node ) && ( node->on_Pending ) )
		{
			error = ERROR_OBJECT_IN_USE;
			break;
		}

		name = *end ? end + 1 : end;
	}

	if ( ( ! node ) && ( ! error ) )
	{
		error = ERROR_OBJECT_NOT_FOUND;
	}

	if ( res2 )
	{
		*res2 = error;
	}

	return ( error ? NULL : node );
}

// --
