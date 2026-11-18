
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
AbortReqOnList( struct List *list, struct IOSana2Req *abort_ioreq, U32 wire_error )
{
struct IOSana2Req *ioreq, *next;
S32 num_aborted;

	num_aborted = 0;

	ioreq = (APTR) IExec->GetHead( list );

	while( ioreq )
	{
		next = (APTR) IExec->GetSucc( (APTR) ioreq );

		// Abort either all requests or a specific one
		if (( abort_ioreq == NULL ) || ( ioreq == abort_ioreq ))
		{
			IExec->Remove( (APTR) ioreq );

			ioreq->ios2_Req.io_Error= IOERR_ABORTED;
			ioreq->ios2_WireError	= wire_error;

			IExec->ReplyMsg( (APTR) ioreq );

			num_aborted++;

			// If we aborted a specific request, we can stop right now.
			if ( abort_ioreq != NULL )
			{
				break;
			}
		}

		ioreq = next;
	}

	return(	num_aborted );
}

// --
