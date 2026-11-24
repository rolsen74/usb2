
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/dos.h>
#include <interfaces/exec.h>
#include <interfaces/timer.h>
#include <stddef.h>
#include "Sana2Commands.h"

// --

extern struct ExecIFace *IExec;
extern struct DOSIFace *IDOS;

#define SAMPLE_HAS( field ) ( stats->s2ts_Actual >= offsetof( struct Sana2ThroughputStats, field ) + sizeof( stats->field ) )

// --

void
Sana2_GetTime( struct TimeVal *time )
{
	struct DateStamp stamp;

	IDOS->DateStamp( & stamp );

	time->Seconds = (uint32) stamp.ds_Days * 86400U + (uint32) stamp.ds_Minute * 60U +
		stamp.ds_Tick / TICKS_PER_SECOND;
	time->Microseconds = ( stamp.ds_Tick % TICKS_PER_SECOND ) * ( 1000000U / TICKS_PER_SECOND );
}

// --

int
Sana2_QueueThroughput( struct List *list, struct IOSana2Req *ioreq, struct TimerIFace *timer,
	int copy_partial )
{
	static const struct Sana2ThroughputStats empty = { 0 };

	struct Sana2ThroughputStats sample;
	struct Sana2ThroughputStats *stats;

	uint32 offset;
	uint32 length;

	stats = ioreq->ios2_StatData;
	offset = offsetof( struct Sana2ThroughputStats, s2ts_StartTime );

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

	length = ( stats->s2ts_Length < sizeof( *stats ) ) ? stats->s2ts_Length : sizeof( *stats );

	IExec->CopyMem( & empty, & sample, sizeof( sample ) );

	if ( timer )
	{
		timer->GetSysTime( & sample.s2ts_StartTime );
	}
	else
	{
		Sana2_GetTime( & sample.s2ts_StartTime );
	}

	sample.s2ts_EndTime = sample.s2ts_StartTime;

	// Older wireless drivers report timestamps only when the field fits.
	if ( ! copy_partial )
	{
		if ( length < offset + sizeof( sample.s2ts_StartTime ) )
		{
			sample.s2ts_StartTime = empty.s2ts_StartTime;
		}

		if ( length < offsetof( struct Sana2ThroughputStats, s2ts_EndTime ) + sizeof( sample.s2ts_EndTime ) )
		{
			sample.s2ts_EndTime = empty.s2ts_EndTime;
		}
	}

	IExec->Forbid();

	if ( length > offset )
	{
		IExec->CopyMem( (uint8 *) & sample + offset, (uint8 *) stats + offset, length - offset );
	}

	stats->s2ts_Actual = length;

	IExec->Permit();

	IExec->AddTail( list, (APTR) ioreq );

	return ( TRUE );
}

// --

void
Sana2_AccountThroughput( struct Sana2ThroughputStats *stats, const struct TimeVal *time,
	uint32 sent, uint32 received )
{
	if ( SAMPLE_HAS( s2ts_EndTime ) )
	{
		stats->s2ts_EndTime = *time;
	}

	if ( SAMPLE_HAS( s2ts_BytesSent ) )
	{
		stats->s2ts_BytesSent.s2q_Low += sent;
		stats->s2ts_BytesSent.s2q_High += ( stats->s2ts_BytesSent.s2q_Low < sent );
	}

	if ( SAMPLE_HAS( s2ts_BytesReceived ) )
	{
		stats->s2ts_BytesReceived.s2q_Low += received;
		stats->s2ts_BytesReceived.s2q_High += ( stats->s2ts_BytesReceived.s2q_Low < received );
	}

	if ( SAMPLE_HAS( s2ts_Updates ) )
	{
		stats->s2ts_Updates.s2q_Low++;
		stats->s2ts_Updates.s2q_High += ( ! stats->s2ts_Updates.s2q_Low );
	}
}

// --

void
Sana2_UpdateThroughput( struct List *list, uint32 sent, uint32 received )
{
	struct IOSana2Req *ioreq;
	struct Sana2ThroughputStats *stats;
	struct TimeVal time;

	for( ioreq = (APTR) IExec->GetHead( list ); ioreq; ioreq = (APTR) IExec->GetSucc( (APTR) ioreq ) )
	{
		if ( ioreq->ios2_Req.io_Command != S2_SAMPLE_THROUGHPUT )
		{
			continue;
		}

		stats = ioreq->ios2_StatData;

		Sana2_GetTime( & time );

		IExec->Forbid();

		Sana2_AccountThroughput( stats, & time, sent, received );

		if ( ( SAMPLE_HAS( s2ts_NotifyMask ) ) && ( stats->s2ts_NotifyTask ) && ( stats->s2ts_NotifyMask ) )
		{
			IExec->Signal( stats->s2ts_NotifyTask, stats->s2ts_NotifyMask );
		}

		IExec->Permit();
	}
}

// --
