
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct FS_ObjNode *_fs_Handle_Rel_Args( struct FS_Struct *fs, struct FS_ObjLock *rel, CONST_STRPTR name )
{
struct FS_ObjNode *ref;
PTR retval;
	
	MYINFO( "PTP-FS : _fs_Handle_Rel_Args" );

	if ( rel )
	{
		ref = rel->Node;
	}
	else
	{
		ref = fs->fs_RootNode;
	}

	if ( name[0] )
	{
		retval = _fs_Node_Find( fs, ref, NULL, (STR) name );
	}
	else
	{
		retval = ref;
	}

	return( retval );
}

// --
