
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Commands.h"
#include "Radio.h"

// --

S32
Cmd_Read( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct BufferManagement *opener;

	if ( ( ! unit->unit_Online ) &&
		( ! ( RTL8188EU_DeferredLink( unit ) && unit->unit_AssocID && ioreq->ios2_PacketType == RTL8188EU_EAPOL &&
			! ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW ) ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		return ( FALSE );
	}

	opener = ioreq->ios2_BufferManagement;

	if ( ( ! ioreq->ios2_Data ) || ( ! opener ) || ( ( ! opener->bm_CopyHook ) && ( ! opener->bm_CopyToBuff ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_BUFF_ERROR;

		return ( FALSE );
	}

	AddTail( & unit->unit_Pending, (PTR) ioreq );

	RTL8188EU_SubmitRX( unit );

	return ( TRUE );
}

// --
