
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/exec.h>
#include "Sana2Commands.h"

// --

extern struct ExecIFace *IExec;

// --

void
Sana2_GetGlobalStats( struct IOSana2Req *ioreq, const struct Sana2DeviceStats *sample )
{
	if ( ! ioreq->ios2_StatData )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	IExec->CopyMem( sample, ioreq->ios2_StatData, sizeof( *sample ) );
}

// --

void
Sana2_GetPacketStats( struct IOSana2Req *ioreq, uint64 received, uint64 sent,
	uint64 bad, uint64 unknown, const struct TimeVal *last_start )
{
	struct Sana2DeviceStats sample;

	static const struct Sana2DeviceStats empty = { 0 };

	sample = empty;
	sample.PacketsReceived = received;
	sample.PacketsSent = sent;
	sample.BadData = bad;
	sample.UnknownTypesReceived = unknown;

	if ( last_start )
	{
		sample.LastStart = *last_start;
	}

	Sana2_GetGlobalStats( ioreq, & sample );
}

// --
