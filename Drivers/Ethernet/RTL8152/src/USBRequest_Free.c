
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
FreeIONode( struct devUnit *unit, struct IONode *node )
{
	if ( node->Active )
	{
		IExec->AbortIO( (PTR) node->USBRequest );

		IExec->WaitIO( (PTR) node->USBRequest );
	}

	if ( node->Sana2IOReq )
	{
		node->Sana2IOReq->ios2_Req.io_Error = IOERR_ABORTED;
		node->Sana2IOReq->ios2_WireError = S2WERR_GENERIC_ERROR;

		ReplySanaRequest( unit, (PTR) node->Sana2IOReq );
	}

	IExec->FreeVec( node );
}

// --
