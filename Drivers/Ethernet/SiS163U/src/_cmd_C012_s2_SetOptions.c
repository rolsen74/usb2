
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

void
Cmd_SetOptions( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct TagItem *tags;
	struct TagItem *tag;

	U32 channel;
	S32 error;

	tags = ioreq->ios2_Data;
	channel = 0;

	if ( ! tags )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	while(( tag = NextTagItem( & tags ) ))
	{
		if ( ( tag->ti_Tag != S2INFO_Channel ) || ( tag->ti_Data < 1 ) || ( tag->ti_Data > SIS163U_CHANNEL_MAX ) )
		{
			ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
			ioreq->ios2_WireError = S2WERR_INVALID_OPTION;

			return;
		}

		channel = tag->ti_Data;
	}

	if ( ! channel )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_MISSING_OPTION;

		return;
	}

	if ( ( ! unit->unit_RadioInitialized ) || ( ! unit->unit_RxReady ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		return;
	}

	SiS163U_StopRX( unit );

	error = SiS163U_SetChannel( unit, channel );

	if ( error )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;

		USBERROR( "SiS163U: could not tune channel %lu (USB error %ld)", channel, error );

		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );
	}
	else
	{
		unit->unit_ManualScan = TRUE;
		unit->unit_RxCheckReads = 0;
	}
}

// --
