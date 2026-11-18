
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void
_fs_Node_Delete( struct FS_Struct *fs, struct FS_ObjNode *node )
{
	struct FS_ObjNode *child;

	if ( ! node )
	{
		return;
	}

	while(( child = (PTR) RemHead( & node->on_Content_List ) ))
	{
		_fs_Node_Delete( fs, child );
	}

	if ( node->on_Name )
	{
		FreeVec( node->on_Name );
	}

	FreeVec( node );
}

// --
