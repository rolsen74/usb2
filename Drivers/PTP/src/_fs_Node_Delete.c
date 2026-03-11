
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _fs_Node_Delete( struct FS_Struct *fs UNUSED, struct FS_ObjNode *node )
{
	MYINFO( "PTP-FS : _fs_Node_Delete" );

	if ( ! node )
	{
		goto bailout;
	}

	if ( node->on_Name )
	{
		FreeVec( node->on_Name );
		node->on_NameLen = 0;
		node->on_Name = NULL;
	}

	// if ( node->DeadMessage )
	// {
	// 	node->DeadMessage->cm_Result = TRUE;
	// 	ReplyMsg( (PTR) node->DeadMessage );
	// 	node->DeadMessage = NULL;
	// }

	FreeVec( node );

bailout:

	return;
}

// --
