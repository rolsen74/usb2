
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
Cmd_SampleThroughput( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct Sana2ThroughputStats *stats;
	struct DateStamp stamp;
	struct TimeVal now;

	U32 length;
	U32 offset;

	stats = ioreq->ios2_StatData;

	if ( ! stats )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return ( FALSE );
	}

	if ( stats->s2ts_Length < offsetof( struct Sana2ThroughputStats, s2ts_NotifyTask ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
		ioreq->ios2_WireError = S2WERR_BAD_STATDATA;

		return ( FALSE );
	}

	DateStamp( & stamp );

	now.Seconds = (U32) stamp.ds_Days * 86400U + (U32) stamp.ds_Minute * 60U + stamp.ds_Tick / TICKS_PER_SECOND;
	now.Microseconds = ( stamp.ds_Tick % TICKS_PER_SECOND ) * ( 1000000U / TICKS_PER_SECOND );
	length = stats->s2ts_Length < sizeof( *stats ) ? stats->s2ts_Length : sizeof( *stats );
	offset = offsetof( struct Sana2ThroughputStats, s2ts_StartTime );

	Forbid();

	if ( length > offset )
	{
		memset( (U8 *) stats + offset, 0, length - offset );
	}

	if ( length >= offset + sizeof( now ) )
	{
		stats->s2ts_StartTime = now;
	}

	if ( length >= offsetof( struct Sana2ThroughputStats, s2ts_EndTime ) + sizeof( now ) )
	{
		stats->s2ts_EndTime = now;
	}

	stats->s2ts_Actual = length;

	Permit();

	AddTail( & unit->unit_Pending, (PTR) ioreq );

	return ( TRUE );
}

// --
