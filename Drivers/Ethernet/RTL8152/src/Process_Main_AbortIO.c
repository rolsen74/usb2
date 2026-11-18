
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
Handle_AbortIOMessages( struct devUnit *unit )
{
	struct AbortIOMessage *msg;
	struct IOSana2Req *ioreq;
	struct IONode *node;

	while(( msg = (PTR) IExec->GetMsg( unit->unit_AbortIOMsgPort ) ))
	{
		ioreq = msg->ioreq;

		IExec->FreeVec( msg );

		if ( ( AbortReqOnList( & unit->unit_RxList, ioreq, 0 ) ) ||
			( AbortReqOnList( & unit->unit_TxList, ioreq, 0 ) ) ||
			( AbortReqOnList( & unit->unit_EventsList, ioreq, 0 ) ) ||
			( AbortReqOnList( & unit->unit_SampleList, ioreq, 0 ) ) )
		{
			continue;
		}

		node = (PTR) IExec->GetHead( & unit->unit_USBBulkOutActiveList );

		while( node )
		{
			if ( ( node->Sana2IOReq == ioreq ) && ( ! node->AbortError ) )
			{
				node->AbortError = IOERR_ABORTED;

				IExec->AbortIO( (PTR) node->USBRequest );

				break;
			}

			node = (PTR) IExec->GetSucc( & node->Node );
		}
	}
}

// --
