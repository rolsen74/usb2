
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

void
Cmd_Sana2Hook( struct EthernetUnit *unit UNUSED, struct IOSana2Req *ioreq )
{
	struct Sana2Hook *hook;
	struct BufferManagement *opener;

	hook = ioreq->ios2_Data;
	opener = ioreq->ios2_BufferManagement;

	if ( ioreq->ios2_DataLength < sizeof( *hook ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
	}
	else if ( ( ! opener ) || ( ! hook ) || ( ! hook->s2h_Methods ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else if ( opener->bm_CopyHook )
	{
		ioreq->ios2_Req.io_Error = IOERR_UNITBUSY;
	}
	else if ( ( ! TagInArray( S2_CopyToBuff, hook->s2h_Methods ) ) ||
		( ! TagInArray( S2_CopyFromBuff, hook->s2h_Methods ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_FUNCTIONS_MISSING;
	}
	else
	{
		opener->bm_CopyHook = & hook->s2h_Hook;
	}
}

// --
