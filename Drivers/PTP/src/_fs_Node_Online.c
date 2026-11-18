
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
	return ( ( ( ! node->on_CameraRoot ) || ( node->on_CameraRoot->on_Camera ) ) &&
		( ( ! node->on_StorageRoot ) || ( ! node->on_StorageRoot->on_StorageRemoved ) ) );
}

// --
