
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _fs_Node_Delete( struct PTP_FSStruct *fs UNUSED, struct FS_ObjNode *node )
{
	if ( ! node )
	{
		goto bailout;
	}

	if ( node->Name )
	{
		FreeVec( node->Name );
		node->NameLen = 0;
		node->Name = NULL;
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
