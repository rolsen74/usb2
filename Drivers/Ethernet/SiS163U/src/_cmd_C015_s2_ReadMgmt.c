
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

struct IOSana2Req *
SiS163U_FindMgmtRead( struct EthernetUnit *unit )
{
	struct IOSana2Req *ioreq;

	ioreq = (PTR) GetHead( & unit->unit_Pending );

	while( ioreq )
	{
		if ( ioreq->ios2_Req.io_Command == S2_READMGMT )
		{
			return ( ioreq );
		}

		ioreq = (PTR) GetSucc( (PTR) ioreq );
	}

	return ( NULL );
}

// --

S32
Cmd_ReadMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	if ( ( ! unit->unit_ManualScan ) || ( ! unit->unit_RadioInitialized ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;
	}
	else if ( ( ! ioreq->ios2_Data ) || ( ! ioreq->ios2_BufferManagement ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else if ( ioreq->ios2_DataLength < SIS163U_WLAN_HEADER )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
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
