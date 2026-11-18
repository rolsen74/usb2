
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
Cmd_Write( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct BufferManagement *opener;

	U32 raw;

	opener = ioreq->ios2_BufferManagement;
	raw = ioreq->ios2_Req.io_Flags & SANA2IOF_RAW;

	if ( ! unit->unit_Online )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;
	}
	else if ( ( raw && ioreq->ios2_DataLength < 14 ) ||
		( ioreq->ios2_DataLength > RTL8188EU_MTU + ( raw ? 14 : 0 ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_MTU_EXCEEDED;
	}
	else if ( ( ! ioreq->ios2_Data ) || ( ! opener ) || ( ( ! opener->bm_CopyHook ) && ( ! opener->bm_CopyFromBuff ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_BUFF_ERROR;
	}
	else
	{
		RTL8188EU_QueueControl( unit, ioreq );

		return ( TRUE );
	}

	return ( FALSE );
}

// --
