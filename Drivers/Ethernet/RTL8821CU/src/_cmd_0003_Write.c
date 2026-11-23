
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

static const U8 Broadcast[RTL8821CU_ADDR_SIZE] = { 255, 255, 255, 255, 255, 255 };

// --

S32
Cmd_Write( struct EthernetUnit *unit, struct BufferManagement *opener, struct IOSana2Req *ioreq )
{
	U32 header;

	if ( ( ! unit->unit_Online ) || ( unit->unit_Detached ) )
	{
		ReportUnitOffline( unit, ioreq );

		return ( FALSE );
	}

	header = ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW ) ? RTL8821CU_ETH_HEADER : 0;

	if ( ( ioreq->ios2_DataLength < header ) || ( ioreq->ios2_DataLength > RTL8821CU_MTU + header ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_MTU_EXCEEDED;

		return ( FALSE );
	}

	if ( ( ! ioreq->ios2_Data ) || ( ( ! opener->bm_CopyHook ) && ( ! opener->bm_CopyFromBuff ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_BUFF_ERROR;

		return ( FALSE );
	}

	if ( ( ioreq->ios2_Req.io_Command == S2_MULTICAST ) &&
		( ( ! ( ioreq->ios2_DstAddr[0] & 1 ) ) || ( RTL8821CU_SameAddress( ioreq->ios2_DstAddr, Broadcast ) ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_BAD_MULTICAST;

		return ( FALSE );
	}

	if ( ioreq->ios2_Req.io_Command == S2_BROADCAST )
	{
		CopyMem( (PTR) Broadcast, ioreq->ios2_DstAddr, sizeof( Broadcast ) );
	}

	ioreq->ios2_Req.io_Flags &= ~IOF_QUICK;

	AddTail( & unit->unit_Pending, (PTR) ioreq );

	return ( TRUE );
}

// --
