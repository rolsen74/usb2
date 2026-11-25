
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
Copy_To_Buffer( struct devUnit *unit UNUSED, struct IOSana2Req *ioreq, APTR buffer )
{
	struct BufferManagement *bm;
	struct Hook *hook;

	bm = ioreq->ios2_BufferManagement;

	if ( ! bm )
	{
		return ( FALSE );
	}

	hook = ( bm->bm_CopyToBuffTag == S2_CopyToBuff ) ? bm->bm_CopyHook : NULL;

	return ( Sana2_CopyPacket( ioreq, hook, bm->bm_CopyToBuff, S2_CopyToBuff,
		ioreq->ios2_Data, buffer, ioreq->ios2_DataLength ) );
}

// --
