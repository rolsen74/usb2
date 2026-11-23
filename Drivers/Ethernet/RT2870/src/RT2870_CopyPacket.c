
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
RT2870_CopyPacket( struct IOSana2Req *ioreq,
	PTR to, PTR from, U32 length, S32 rx )
{
	struct BufferManagement *opener;

	PTR callback;
	ULONG method;

	opener = ioreq->ios2_BufferManagement;

	if ( ! opener )
	{
		return ( FALSE );
	}

	callback = rx ? opener->bm_CopyToBuff : opener->bm_CopyFromBuff;
	method = rx ? S2_CopyToBuff : S2_CopyFromBuff;

	return ( Sana2_CopyPacket( ioreq, opener->bm_CopyHook, callback, method, to, from, length ) != 0 );
}

// --
