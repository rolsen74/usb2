
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

#define COMPLETE_FIELD( member ) \
	( stats->s2ts_Actual >= offsetof( struct Sana2ThroughputStats, member ) + sizeof( stats->member ) )

// --

void
RTL8188EU_UpdateThroughput( struct EthernetUnit *unit, U32 sent, U32 received )
{
	struct IOSana2Req *request;
	struct Sana2ThroughputStats *stats;
	struct DateStamp stamp;

	request = (PTR) GetHead( & unit->unit_Pending );

	while( request )
	{
		if ( request->ios2_Req.io_Command == S2_SAMPLE_THROUGHPUT )
		{
			stats = request->ios2_StatData;

			DateStamp( & stamp );

			Forbid();

			if ( COMPLETE_FIELD( s2ts_EndTime ) )
			{
				stats->s2ts_EndTime.Seconds = (U32) stamp.ds_Days * 86400U +
					(U32) stamp.ds_Minute * 60U + stamp.ds_Tick / TICKS_PER_SECOND;
				stats->s2ts_EndTime.Microseconds = ( stamp.ds_Tick % TICKS_PER_SECOND ) *
					( 1000000U / TICKS_PER_SECOND );
			}

			if ( COMPLETE_FIELD( s2ts_BytesSent ) )
			{
				stats->s2ts_BytesSent.s2q_Low += sent;
				stats->s2ts_BytesSent.s2q_High += ( stats->s2ts_BytesSent.s2q_Low < sent );
			}

			if ( COMPLETE_FIELD( s2ts_BytesReceived ) )
			{
				stats->s2ts_BytesReceived.s2q_Low += received;
				stats->s2ts_BytesReceived.s2q_High += ( stats->s2ts_BytesReceived.s2q_Low < received );
			}

			if ( COMPLETE_FIELD( s2ts_Updates ) )
			{
				stats->s2ts_Updates.s2q_Low++;
				stats->s2ts_Updates.s2q_High += ( ! stats->s2ts_Updates.s2q_Low );
			}

			if ( ( COMPLETE_FIELD( s2ts_NotifyMask ) ) && ( stats->s2ts_NotifyTask ) && ( stats->s2ts_NotifyMask ) )
			{
				Signal( stats->s2ts_NotifyTask, stats->s2ts_NotifyMask );
			}

			Permit();
		}

		request = (PTR) GetSucc( (PTR) request );
	}
}

// --
