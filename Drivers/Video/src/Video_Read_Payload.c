
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static void
ReportFrameLength( struct VideoUnit *unit, U32 bytes, U32 expected )
{
	unit->CaptureStats.FrameLengthErrors++;

	if ( unit->FrameReports < 4 )
	{
		unit->FrameReports++;

		DebugPrintF( "USB Video: %04lx:%04lx unit %lu discarded frame, %lu bytes, expected %lu\n",
			unit->Format.VendorID, unit->Format.DeviceID, unit->UnitNr, bytes, expected );
	}
}

// --

static S32
FillShortYUY2Tail( struct VideoUnit *unit, U8 *frame, U32 expected )
{
	U32 missing;
	U32 stride;
	U32 index;

	missing = expected - unit->FrameUsed;
	stride = unit->Format.Width * 2;

	if ( ( unit->Format.VendorID != 0x18ec ) || ( unit->Format.DeviceID != 0x3299 ) ||
		( missing > stride ) || ( missing & 1 ) )
	{
		return ( FALSE );
	}

	for ( index = unit->FrameUsed; index < expected; index += 2 )
	{
		frame[index] = 16;
		frame[index + 1] = 128;
	}

	unit->FrameUsed = expected;

	return ( TRUE );
}

// --

SEC_CODE S32
Video_Read_Payload ( struct VideoUnit *unit, U8 *data, U32 length )
{
	struct IOStdReq *read;

	U32 header;
	U32 payload;
	U32 expected;
	U32 limit;
	U32 pts;
	S32 newFrame;
	U8 flags;
	U8 frameid;

	read = unit->ReadActive;

	// A zero-length USB completion can terminate a bulk payload.
	if ( ( ! read ) || ( ! length ) )
	{
		return ( TRUE );
	}

	if ( ( ! data ) || ( length < 2 ) ||
		( data[0] < 2 ) || ( data[0] > length ) ||
		( ( unit->OV534 ) && ( ( data[0] != 12 ) || ( ! ( data[1] & VIDEO_PAYLOAD_PTS ) ) ) ) )
	{
		if ( data )
		{
			unit->CaptureStats.InvalidPayloadHeaders++;
		}

		// Keep a known OV534 identity so the next frame can clear discard.
		if ( ! unit->OV534 )
		{
			unit->FrameStarted = FALSE;
		}

		unit->FrameDiscard = TRUE;
		unit->FrameUsed = 0;

		return ( TRUE );
	}

	header = data[0];
	flags = data[1];
	frameid = flags & VIDEO_PAYLOAD_FID;
	pts = unit->OV534 ? Video_LE32( data + 2 ) : 0;
	expected = ( unit->Format.Format == VIDEO_FORMAT_YUY2 ) ? unit->Format.MaxFrameSize : 0;
	newFrame = ( ! unit->FrameStarted ) || ( frameid != unit->FrameID ) || ( pts != unit->FramePTS );

	if ( ( unit->FrameStarted ) && ( newFrame ) )
	{
		// Some cameras delimit frames with FID and omit EOF.
		if ( ( ! unit->FrameDiscard ) && ( unit->FrameUsed ) )
		{
			if ( ( ! expected ) || ( unit->FrameUsed == expected ) )
			{
				Video_Read_Reply( unit, 0 );

				// The same packet starts the next frame.
				return ( FALSE );
			}

			ReportFrameLength( unit, unit->FrameUsed, expected );
		}

		unit->FrameUsed = 0;
		unit->FrameDiscard = FALSE;
	}

	if ( newFrame )
	{
		unit->CaptureStats.PayloadFrameStarts++;
	}

	unit->FrameID = frameid;
	unit->FramePTS = pts;
	unit->FrameStarted = TRUE;

	if ( flags & VIDEO_PAYLOAD_ERR )
	{
		unit->CaptureStats.PayloadErrorFlags++;
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

		return ( TRUE );
	}

	payload = length - header;

	if ( ( expected ) && ( read->io_Length < expected ) )
	{
		Video_Read_Reply( unit, IOERR_BADLENGTH );

		unit->FrameDiscard = ! ( flags & VIDEO_PAYLOAD_EOF );
		unit->FrameStarted = unit->FrameDiscard;

		return ( TRUE );
	}

	limit = expected ? expected : MIN( read->io_Length, unit->Format.MaxFrameSize );

	if ( payload > limit - unit->FrameUsed )
	{
		if ( read->io_Length < unit->Format.MaxFrameSize )
		{
			Video_Read_Reply( unit, IOERR_BADLENGTH );
		}
		else
		{
			ReportFrameLength( unit, unit->FrameUsed + payload, limit );

			unit->FrameUsed = 0;
		}

		unit->FrameDiscard = ! ( flags & VIDEO_PAYLOAD_EOF );
		unit->FrameStarted = unit->FrameDiscard;

		return ( TRUE );
	}

	CopyMem( data + header, (U8 *) read->io_Data + unit->FrameUsed, payload );

	unit->FrameUsed += payload;

	if ( flags & VIDEO_PAYLOAD_EOF )
	{
		unit->FrameStarted = FALSE;

		if ( unit->FrameUsed )
		{
			if ( ( expected ) && ( unit->FrameUsed != expected ) &&
				( ! FillShortYUY2Tail( unit, read->io_Data, expected ) ) )
			{
				ReportFrameLength( unit, unit->FrameUsed, expected );

				unit->FrameUsed = 0;
			}
			else
			{
				Video_Read_Reply( unit, 0 );
			}
		}
	}

	return ( TRUE );
}

// --
