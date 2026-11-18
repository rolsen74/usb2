
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void
Video_Read_Payload ( struct VideoUnit *unit, U8 *data, U32 length )
{
	struct IOStdReq *read;

	U32 header;
	U32 payload;
	U8 flags;
	U8 frameid;

	read = unit->ReadActive;

	// A zero-length USB completion can terminate a bulk payload.
	if ( ( ! read ) || ( ! length ) )
	{
		return;
	}

	if ( ( ! data ) || ( length < 2 ) ||
		( data[0] < 2 ) || ( data[0] > length ) )
	{
		unit->FrameStarted = FALSE;
		unit->FrameDiscard = TRUE;
		unit->FrameUsed = 0;

		return;
	}

	header = data[0];
	flags = data[1];
	frameid = flags & VIDEO_PAYLOAD_FID;

	if ( ( unit->FrameStarted ) && ( frameid != unit->FrameID ) )
	{
		// Some cameras delimit frames with FID and omit EOF.
		if ( ( ! unit->FrameDiscard ) && ( unit->FrameUsed ) )
		{
			Video_Read_Reply( unit, 0 );
			return;
		}

		unit->FrameDiscard = FALSE;
	}

	unit->FrameID = frameid;
	unit->FrameStarted = TRUE;

	if ( flags & VIDEO_PAYLOAD_ERR )
	{
		unit->FrameDiscard = TRUE;
		unit->FrameUsed = 0;
	}

	if ( unit->FrameDiscard )
	{
		if ( flags & VIDEO_PAYLOAD_EOF )
		{
			unit->FrameStarted = FALSE;
			unit->FrameDiscard = FALSE;
		}

		return;
	}

	payload = length - header;

	if ( payload > read->io_Length - unit->FrameUsed )
	{
		Video_Read_Reply( unit, IOERR_BADLENGTH );
		return;
	}

	CopyMem( data + header, (U8 *) read->io_Data + unit->FrameUsed, payload );
	unit->FrameUsed += payload;

	if ( flags & VIDEO_PAYLOAD_EOF )
	{
		unit->FrameStarted = FALSE;

		if ( unit->FrameUsed )
		{
			Video_Read_Reply( unit, 0 );
		}
	}
}

// --
