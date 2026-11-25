
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
	struct SanaRequest *submission;
	struct IOSana2Req *ioreq;
	struct IONode *node;

	while ( TRUE )
	{
		ioreq = NULL;

		IExec->ObtainSemaphore( & unit->unit_DispatchLock );

		for ( submission = (PTR) IExec->GetHead( & unit->unit_Requests ); submission;
			submission = (PTR) IExec->GetSucc( & submission->Node ) )
		{
			if ( ( submission->Started ) && ( submission->Cancelled ) && ( ! submission->AbortHandled ) )
			{
				submission->AbortHandled = TRUE;
				ioreq = submission->Request;
				break;
			}
		}

		IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

		if ( ! ioreq )
		{
			break;
		}

		if ( ( AbortReqOnList( unit, & unit->unit_RxList, ioreq, 0 ) ) ||
			( AbortReqOnList( unit, & unit->unit_TxList, ioreq, 0 ) ) ||
			( AbortReqOnList( unit, & unit->unit_EventsList, ioreq, 0 ) ) ||
			( AbortReqOnList( unit, & unit->unit_SampleList, ioreq, 0 ) ) )
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
