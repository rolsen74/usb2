
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

int
Copy_From_Buffer( struct devUnit *unit UNUSED, struct IOSana2Req *ioreq, APTR buffer )
{
	struct BufferManagement *bm;
	struct Hook *hook;

	bm = ioreq->ios2_BufferManagement;

	if ( ! bm )
	{
		return ( FALSE );
	}

	hook = ( bm->bm_CopyFromBuffTag == S2_CopyFromBuff ) ? bm->bm_CopyHook : NULL;

	return ( Sana2_CopyPacket( ioreq, hook, bm->bm_CopyFromBuff, S2_CopyFromBuff,
		buffer, ioreq->ios2_Data, ioreq->ios2_DataLength ) );
}

// --
