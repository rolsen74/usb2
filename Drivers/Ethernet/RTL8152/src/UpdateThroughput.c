
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

#define SANA2_SAMPLE_HAS( stats, member ) \
	( (stats)->s2ts_Actual >= offsetof( struct Sana2ThroughputStats, member ) + sizeof( (stats)->member ) )

void
UpdateThroughput( struct devUnit *unit, U32 sent, U32 received )
{
	struct IOSana2Req *ioreq;
	struct Sana2ThroughputStats *stats;
	struct TimeVal now;

	ioreq = (PTR) IExec->GetHead( & unit->unit_SampleList );

	if ( ! ioreq )
	{
		return;
	}

	ITimer->GetSysTime( & now );

	while( ioreq )
	{
		stats = ioreq->ios2_StatData;

		IExec->Forbid();

		Sana2_AccountThroughput( stats, & now, sent, received );

		if ( ( SANA2_SAMPLE_HAS( stats, s2ts_EndTime ) ) &&
			( stats->s2ts_NotifyTask ) && ( stats->s2ts_NotifyMask ) )
		{
			IExec->Signal( stats->s2ts_NotifyTask, stats->s2ts_NotifyMask );
		}

		IExec->Permit();

		ioreq = (PTR) IExec->GetSucc( (PTR) ioreq );
	}
}

#undef SANA2_SAMPLE_HAS

// --
