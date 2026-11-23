
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

	bm = ioreq->ios2_BufferManagement;
	ioreq->ios2_WireError = 0;

	if ( Sana2_SetCopyHook( ioreq, bm ? & bm->bm_CopyHook : NULL, TRUE ) )
	{
		bm->bm_CopyToBuff = NULL;
		bm->bm_CopyFromBuff = NULL;
		bm->bm_CopyToBuffTag = S2_CopyToBuff;
		bm->bm_CopyFromBuffTag = S2_CopyFromBuff;
	}

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
