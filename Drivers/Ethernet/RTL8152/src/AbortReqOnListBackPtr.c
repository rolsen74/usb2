
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

S32
AbortReqOnListBackPtr( struct devUnit *unit UNUSED, struct List *list, struct BufferManagement *back_ptr, U32 wire_error )
{
struct IOSana2Req *ioreq;
struct IOSana2Req *next;
U32 num_aborted;

	num_aborted = 0;

	ioreq = (APTR) IExec->GetHead( list );

	while( ioreq )
	{
		next = (APTR) IExec->GetSucc( (APTR) ioreq );

		// A non-null cookie selects every queued request for the opener.
		if (( back_ptr == NULL ) || ( ioreq->ios2_BufferManagement == (APTR)back_ptr ))
		{
			IExec->Remove( (APTR) ioreq );

			ioreq->ios2_Req.io_Error = ( wire_error == S2WERR_UNIT_OFFLINE ) ?
				S2ERR_OUTOFSERVICE : IOERR_ABORTED;
			ioreq->ios2_WireError = wire_error;

			IExec->ReplyMsg( (APTR) ioreq );

			num_aborted++;
		}

		ioreq = next;
	}

	return(	num_aborted );
}

// --
