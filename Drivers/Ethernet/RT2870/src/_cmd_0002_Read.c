
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

S32
Cmd_Read( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct BufferManagement *opener;

	S32 eapol;

	opener = ioreq->ios2_BufferManagement;
	eapol = ioreq->ios2_PacketType == RT2870_EAPOL_TYPE;

	if ( ( eapol ) ? ( ! unit->unit_Channel || ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW ) ) : ( ! unit->unit_Online ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;
	}
	else if ( ! ioreq->ios2_Data )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else if ( ( ! eapol ) && ( ( ! opener ) || ( ( ! opener->bm_CopyHook ) && ( ! opener->bm_CopyToBuff ) ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_BUFF_ERROR;
	}
	else
	{
		AddTail( & unit->unit_Pending, (PTR) ioreq );

		Signal( unit->unit_TaskAdr, SIGBREAKF_CTRL_E );

		return ( TRUE );
	}

	return ( FALSE );
}

// --
