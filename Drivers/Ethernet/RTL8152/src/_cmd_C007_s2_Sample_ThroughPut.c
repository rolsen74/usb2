
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
s2_Sample_ThroughPut( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	struct Sana2ThroughputStats *output;
	struct Sana2ThroughputStats stats;

	U32 offset;

	output = ioreq->ios2_StatData;
	offset = offsetof( struct Sana2ThroughputStats, s2ts_StartTime );
	ioreq->ios2_WireError = 0;

	if ( ! output )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else if ( output->s2ts_Length < offsetof( struct Sana2ThroughputStats, s2ts_NotifyTask ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
		ioreq->ios2_WireError = S2WERR_BAD_STATDATA;
	}
	else
	{
		memset( & stats, 0, sizeof( stats ) );

		stats.s2ts_Actual = output->s2ts_Length < sizeof( stats ) ? output->s2ts_Length : sizeof( stats );

		ITimer->GetSysTime( & stats.s2ts_StartTime );

		stats.s2ts_EndTime = stats.s2ts_StartTime;

		IExec->Forbid();

		// Keep the caller's notification task and signal mask.
		if ( stats.s2ts_Actual > offset )
		{
			memcpy( (U8 *) output + offset, (U8 *) & stats + offset, stats.s2ts_Actual - offset );
		}

		output->s2ts_Actual = stats.s2ts_Actual;

		IExec->Permit();

		IExec->AddTail( & unit->unit_SampleList, (PTR) ioreq );

		return;
	}

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
