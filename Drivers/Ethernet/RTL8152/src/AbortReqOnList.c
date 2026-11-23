
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
	return ( Sana2_AbortPending( list, abort_ioreq ? & abort_ioreq->ios2_Req : NULL,
		NULL, IOERR_ABORTED, wire_error ) );
}

// --
