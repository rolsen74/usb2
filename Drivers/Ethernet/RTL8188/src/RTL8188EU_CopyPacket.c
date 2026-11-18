
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

S32
RTL8188EU_CopyPacket( struct EthernetUnit *unit UNUSED, struct IOSana2Req *ioreq,
	PTR to, PTR from, U32 length, S32 rx )
{
	struct SANA2CopyHookMsg copy;
	struct BufferManagement *opener;

	PTR callback;

	opener = ioreq->ios2_BufferManagement;

	if ( ! opener )
	{
		return ( FALSE );
	}

	if ( opener->bm_CopyHook )
	{
		copy.schm_Method = rx ? S2_CopyToBuff : S2_CopyFromBuff;
		copy.schm_MsgSize = sizeof( copy );
		copy.schm_To = to;
		copy.schm_From = from;
		copy.schm_Size = length;

		return ( CallHookPkt( opener->bm_CopyHook, ioreq, & copy ) != 0 );
	}

	callback = rx ? opener->bm_CopyToBuff : opener->bm_CopyFromBuff;

	return ( callback && EmulateTags( callback, ET_RegisterA0, to,
		ET_RegisterA1, from, ET_RegisterD0, length, TAG_END ) );
}

// --
