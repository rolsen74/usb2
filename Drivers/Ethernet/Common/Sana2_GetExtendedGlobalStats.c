
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/exec.h>
#include <stddef.h>
#include "Sana2Commands.h"

// --

extern struct ExecIFace *IExec;

// --

void
Sana2_GetExtendedGlobalStats( struct IOSana2Req *ioreq, const struct Sana2ExtDeviceStats *sample,
	uint32 available )
{
	struct Sana2ExtDeviceStats *output;

	uint32 length;
	uint32 offset;

	output = ioreq->ios2_StatData;
	offset = offsetof( struct Sana2ExtDeviceStats, s2xds_PacketsReceived );

	if ( ! output )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	if ( output->s2xds_Length < offset )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
		ioreq->ios2_WireError = S2WERR_BAD_STATDATA;

		return;
	}

	length = ( output->s2xds_Length < available ) ? output->s2xds_Length : available;

	IExec->CopyMem( (const uint8 *) sample + offset, (uint8 *) output + offset, length - offset );

	output->s2xds_Actual = length;
}

// --

void
Sana2_GetExtendedPacketStats( struct IOSana2Req *ioreq, uint64 received, uint64 sent,
	uint64 bad, uint64 unknown, int partial )
{
	struct Sana2ExtDeviceStats *output;
	struct Sana2ExtDeviceStats sample;

	static const struct Sana2ExtDeviceStats empty = { 0 };

	uint32 length;

	output = ioreq->ios2_StatData;
	length = output ? output->s2xds_Length : 0;

	IExec->CopyMem( & empty, & sample, sizeof( sample ) );

	if ( ( partial ) || ( length >= offsetof( struct Sana2ExtDeviceStats, s2xds_PacketsSent ) ) )
	{
		sample.s2xds_PacketsReceived.s2q_High = received >> 32;
		sample.s2xds_PacketsReceived.s2q_Low = received;
	}

	if ( ( partial ) || ( length >= offsetof( struct Sana2ExtDeviceStats, s2xds_BadData ) ) )
	{
		sample.s2xds_PacketsSent.s2q_High = sent >> 32;
		sample.s2xds_PacketsSent.s2q_Low = sent;
	}

	sample.s2xds_BadData.s2q_High = bad >> 32;
	sample.s2xds_BadData.s2q_Low = bad;

	if ( ( partial ) || ( length >= offsetof( struct Sana2ExtDeviceStats, s2xds_Reconfigurations ) ) )
	{
		sample.s2xds_UnknownTypesReceived.s2q_High = unknown >> 32;
		sample.s2xds_UnknownTypesReceived.s2q_Low = unknown;
	}

	Sana2_GetExtendedGlobalStats( ioreq, & sample, sizeof( sample ) );
}

// --
