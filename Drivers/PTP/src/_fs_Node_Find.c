
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
_fs_Node_Find( struct FS_ObjNode *node, STR name, U32 length )
{
	struct FS_ObjNode *child;

	for( child = (PTR) GetHead( & node->on_Content_List ); child;
		child = (PTR) GetSucc( & child->on_Node ) )
	{
		if ( ( child->on_NameLen == length ) && ( ! Strnicmp( child->on_Name, name, length ) ) )
		{
			return ( child );
		}
	}

	return ( NULL );
}

// --
