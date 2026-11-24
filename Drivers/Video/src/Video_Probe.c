/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static U32
ProbeLength( struct VideoUnit *unit )
{
	const U8 *desc;
	U32 remaining;
	U32 length;
	U32 index;
	U32 version;
	S32 control;

	desc = (PTR) unit->StartMsg->ConfigDescriptors;
	remaining = desc[2] | ( (U32) desc[3] << 8 );
	control = FALSE;

	while ( remaining >= 2 )
	{
		length = desc[0];

		if ( ( length < 2 ) || ( length > remaining ) )
		{
			break;
		}

		if ( ( desc[1] == DSCTYPE_Interface ) && ( length >= sizeof( struct USB2_Interface_Desc ) ) )
		{
			control = ( desc[3] == 0 ) && ( desc[5] == 0x0e ) && ( desc[6] == 0x01 );
		}
		else if ( ( control ) && ( length >= 12 ) && ( desc[1] == 0x24 ) &&
			( desc[2] == 0x01 ) && ( desc[11] <= length - 12 ) )
		{
			for ( index = 0; index < desc[11]; index++ )
			{
				if ( desc[12 + index] == unit->InterfaceNr )
				{
					version = desc[3] | ( (U32) desc[4] << 8 );

					if ( version >= 0x0150 )
					{
						return ( 48 );
					}
					else if ( version >= 0x0110 )
					{
						return ( 34 );
					}
					else if ( version >= 0x0100 )
					{
						return ( 26 );
					}
				}
			}
		}

		desc += length;
		remaining -= length;
	}

	return ( 0 );
}

// --

static S32
ProbeFailed( struct VideoUnit *unit, const char *stage, U32 request, U32 reported )
{
	struct USB2_IORequest *ioreq;

	ioreq = unit->Control->IORequests[0];

	DebugPrintF( "USB Video: %04lx:%04lx unit %lu %s failed, interval %lu/%lu, USB error %ld\n",
		unit->Format.VendorID, unit->Format.DeviceID, unit->UnitNr, stage,
		request, reported, (S32) ioreq->io_Error );

	if ( ioreq->io_Error == USB2Err_Host_Stall )
	{
		USB2_EPRes_Destall( unit->Control );
	}

	return ( FALSE );
}

// --

SEC_CODE S32
Video_Probe( struct VideoUnit *unit )
{
U8 probe[sizeof( unit->Commit )];
U8 len[2];
U32 size;
U32 interval;
U32 requested;
U32 index;
U32 frame_size;
U32 payload_size;
U64 raw_size;

	size = unit->ProbeLength;

	if ( ! size )
	{
		size = ProbeLength( unit );
	}

	if ( ! size )
	{
		if ( Video_Control( unit, REQTYPE_Read | REQTYPE_Class | REQTYPE_Interface,
			0x85, 0x0100, len, 2 ) )
		{
			size = len[0] | ( (U32) len[1] << 8 );
		}
		else
		{
			if ( ( unit->Control->IORequests[0]->io_Error ) &&
			( unit->Control->IORequests[0]->io_Error != USB2Err_Host_Stall ) )
			{
				return ( FALSE );
			}

			USB2_EPRes_Destall( unit->Control );

			size = 26;
		}
	}

	if ( ( size < 26 ) || ( size > sizeof( probe ) ) )
	{
		return ( FALSE );
	}

	for( index = 0; index < sizeof( probe ); index++ )
	{
		probe[index] = 0;
	}

	if ( ! Video_Control( unit, REQTYPE_Read | REQTYPE_Class | REQTYPE_Interface,
		0x81, 0x0100, probe, size ) )
	{
		if ( ( unit->Control->IORequests[0]->io_Error ) &&
			( unit->Control->IORequests[0]->io_Error != USB2Err_Host_Stall ) )
		{
			return ( FALSE );
		}

		USB2_EPRes_Destall( unit->Control );

		for( index = 0; index < sizeof( probe ); index++ )
		{
			probe[index] = 0;
		}
	}

	probe[0] = 1;
	probe[1] = 0;
	probe[2] = unit->FormatIndex;
	probe[3] = unit->FrameIndex;
	requested = unit->Format.FrameInterval;
	probe[4] = requested;
	probe[5] = requested >> 8;
	probe[6] = requested >> 16;
	probe[7] = requested >> 24;

	// Let the camera recalculate the sizes for the newly selected mode.
	for ( index = 18; index < 26; index++ )
	{
		probe[index] = 0;
	}

	if ( ! Video_Control( unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x01, 0x0100, probe, size ) )
	{
		return ( ProbeFailed( unit, "SET_CUR(PROBE)", requested, 0 ) );
	}

	if ( ! Video_Control( unit, REQTYPE_Read | REQTYPE_Class | REQTYPE_Interface,
		0x81, 0x0100, probe, size ) )
	{
		return ( ProbeFailed( unit, "GET_CUR(PROBE)", requested, 0 ) );
	}

	if ( ( probe[2] != unit->FormatIndex ) || ( probe[3] != unit->FrameIndex ) )
	{
		return ( ProbeFailed( unit, "probe format/frame", requested, Video_LE32( & probe[4] ) ) );
	}

	interval = Video_LE32( & probe[4] );

	frame_size = Video_LE32( & probe[18] );

	payload_size = Video_LE32( & probe[22] );

	if ( unit->Format.Format == VIDEO_FORMAT_YUY2 )
	{
		raw_size = (U64) unit->Format.Width * unit->Format.Height * 2;

		if ( ( ! raw_size ) || ( raw_size > VIDEO_MAX_FRAME_SIZE ) )
		{
			return ( FALSE );
		}

		// Packed YUY2 has a fixed frame size regardless of the probe value.
		frame_size = raw_size;
	}
	else if ( unit->Format.MaxFrameSize <= VIDEO_MAX_FRAME_SIZE )
	{
		frame_size = MAX( frame_size, unit->Format.MaxFrameSize );
	}

	if ( ( ! interval ) || ( ! frame_size ) || ( frame_size > VIDEO_MAX_FRAME_SIZE ) ||
		( payload_size < 2 ) || ( payload_size > VIDEO_READ_SIZE ) )
	{
		return ( ProbeFailed( unit, "probe values", requested, interval ) );
	}

	unit->Format.FrameInterval = interval;
	unit->Format.MaxFrameSize = frame_size;
	unit->PayloadSize = payload_size;
	unit->ProbeLength = size;

	CopyMem( probe, unit->Commit, size );

	return ( TRUE );
}

// --
