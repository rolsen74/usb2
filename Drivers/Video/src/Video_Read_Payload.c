
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
	if ( unit->FrameReports < 4 )
	{
		unit->FrameReports++;

		DebugPrintF( "USB Video: %04lx:%04lx unit %lu discarded frame, %lu bytes, expected %lu\n",
			unit->Format.VendorID, unit->Format.DeviceID, unit->UnitNr, bytes, expected );
	}
}

// --

static S32
IsBottomUpYUY2( const struct VideoUnit *unit )
{
	return ( ( unit->Format.Format == VIDEO_FORMAT_YUY2 ) &&
		( unit->Format.VendorID == 0x18ec ) && ( unit->Format.DeviceID == 0x3299 ) );
}

// --

static void
CopyFramePayload( struct VideoUnit *unit, U8 *dest, const U8 *data, U32 length )
{
	U32 stride;
	U32 row;
	U32 column;
	U32 bytes;
	U32 offset;

	if ( ! IsBottomUpYUY2( unit ) )
	{
		CopyMem( data, dest + unit->FrameUsed, length );

		return;
	}

	// This camera sends YUY2 scanlines from bottom to top.
	stride = unit->Format.Width * 2;
	row = unit->FrameUsed / stride;
	column = unit->FrameUsed % stride;

	while ( length )
	{
		bytes = MIN( stride - column, length );
		offset = ( unit->Format.Height - 1 - row ) * stride + column;

		CopyMem( data, dest + offset, bytes );

		data += bytes;
		length -= bytes;
		row++;
		column = 0;
	}
}

// --

static S32
FillShortYUY2Tail( struct VideoUnit *unit, U8 *frame, U32 expected )
{
	U32 missing;
	U32 stride;
	U32 offset;
	U32 index;

	missing = expected - unit->FrameUsed;
	stride = unit->Format.Width * 2;

	if ( ( ! IsBottomUpYUY2( unit ) ) || ( missing > stride ) || ( missing & 1 ) )
	{
		return ( FALSE );
	}

	// The last transmitted scanline is at the top of the flipped frame.
	offset = stride - missing;

	for ( index = offset; index < stride; index += 2 )
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
	U8 flags;
	U8 frameid;

	read = unit->ReadActive;

	// A zero-length USB completion can terminate a bulk payload.
	if ( ( ! read ) || ( ! length ) )
	{
		return ( TRUE );
	}

	if ( ( ! data ) || ( length < 2 ) ||
		( data[0] < 2 ) || ( data[0] > length ) )
	{
		unit->FrameStarted = FALSE;
		unit->FrameDiscard = TRUE;
		unit->FrameUsed = 0;

		return ( TRUE );
	}

	header = data[0];
	flags = data[1];
	frameid = flags & VIDEO_PAYLOAD_FID;
	expected = ( unit->Format.Format == VIDEO_FORMAT_YUY2 ) ? unit->Format.MaxFrameSize : 0;

	if ( ( unit->FrameStarted ) && ( frameid != unit->FrameID ) )
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

	limit = expected ? expected : read->io_Length;

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

	CopyFramePayload( unit, read->io_Data, data + header, payload );

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
