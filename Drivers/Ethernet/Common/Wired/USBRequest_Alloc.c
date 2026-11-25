
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct IONode *
AllocIONode( struct USB2_EPResource *resource, U32 index )
{
	struct IONode *node;

	node = IExec->AllocVecTags( sizeof( *node ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! node )
	{
		return ( NULL );
	}

	node->Buffer = resource->Buffers[index];
	node->BufferSize = resource->BufferSize;
	node->USBRequest = resource->IORequests[index];
	node->USBRequest->io_UserData = node;

	return ( node );
}

// --
