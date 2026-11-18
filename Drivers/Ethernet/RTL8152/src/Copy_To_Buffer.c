
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
struct SANA2CopyHookMsg s2_CopyHookMsg;
struct BufferManagement *bm;
int data_copied;

	bm = (APTR) ioreq->ios2_BufferManagement;

	if ( ! bm )
	{
		return ( FALSE );
	}

	if ( bm->bm_CopyToBuffTag == S2_CopyToBuff )
    {
		s2_CopyHookMsg.schm_Method  = bm->bm_CopyToBuffTag;
	    s2_CopyHookMsg.schm_MsgSize = sizeof( struct SANA2CopyHookMsg );
	    s2_CopyHookMsg.schm_To	    = ioreq->ios2_Data;
	    s2_CopyHookMsg.schm_From    = buffer;
	    s2_CopyHookMsg.schm_Size    = ioreq->ios2_DataLength;

		data_copied = (int)( IUtility->CallHookPkt( bm->bm_CopyHook, ioreq, & s2_CopyHookMsg ) != 0 );
    }
	else if ( bm->bm_CopyToBuff )
	{
		data_copied = IExec->EmulateTags( bm->bm_CopyToBuff,
		    ET_RegisterA0, ioreq->ios2_Data,
		    ET_RegisterA1, buffer,
		    ET_RegisterD0, ioreq->ios2_DataLength,
		    TAG_END
	    );
	}
	else
	{
		data_copied = FALSE;
	}

	return ( data_copied );
}

// --
