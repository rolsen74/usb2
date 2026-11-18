
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
s2_Sana2Hook( struct devUnit *unit UNUSED, struct IOSana2Req *ioreq )
{
	struct BufferManagement *bm;
	struct Sana2Hook *s2_hook;

	bm = ioreq->ios2_BufferManagement;
	s2_hook = ioreq->ios2_Data;

	ioreq->ios2_WireError = 0;

	if ( ioreq->ios2_DataLength < sizeof( *s2_hook ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
	}
	else if ( ( ! bm ) || ( ! s2_hook ) || ( ! s2_hook->s2h_Methods ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else if ( ( ! IUtility->TagInArray( S2_CopyToBuff, s2_hook->s2h_Methods ) ) ||
		( ! IUtility->TagInArray( S2_CopyFromBuff, s2_hook->s2h_Methods ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_FUNCTIONS_MISSING;
	}
	else
	{
		bm->bm_CopyHook = & s2_hook->s2h_Hook;
		bm->bm_CopyToBuff = NULL;
		bm->bm_CopyFromBuff = NULL;
		bm->bm_CopyToBuffTag = S2_CopyToBuff;
		bm->bm_CopyFromBuffTag = S2_CopyFromBuff;
	}

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
