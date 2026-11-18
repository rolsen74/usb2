/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32
Video_Probe( struct VideoUnit *unit )
{
U8 probe[48];
U8 len[2];
U32 size;
U32 interval;
U32 index;
U32 frame_size;
U32 payload_size;

	size = 26;

	if ( Video_Control( unit, REQTYPE_Read | REQTYPE_Class | REQTYPE_Interface,
		0x85, 0x0100, len, 2 ) )
	{
		size = len[0] | ( (U32) len[1] << 8 );
	}
	else
	{
		USB2_EPRes_Destall( unit->Control );
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
	interval = unit->Format.FrameInterval;
	probe[4] = interval;
	probe[5] = interval >> 8;
	probe[6] = interval >> 16;
	probe[7] = interval >> 24;

	if ( ! Video_Control( unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x01, 0x0100, probe, size ) )
	{
		return ( FALSE );
	}

	if ( ! Video_Control( unit, REQTYPE_Read | REQTYPE_Class | REQTYPE_Interface,
		0x81, 0x0100, probe, size ) )
	{
		return ( FALSE );
	}

	if ( ( probe[2] != unit->FormatIndex ) || ( probe[3] != unit->FrameIndex ) )
	{
		return ( FALSE );
	}

	interval = Video_LE32( & probe[4] );

	frame_size = Video_LE32( & probe[18] );

	payload_size = Video_LE32( & probe[22] );

	if ( ! frame_size )
	{
		frame_size = unit->Format.MaxFrameSize;
	}

	if ( ( ! interval ) || ( ! frame_size ) || ( frame_size > VIDEO_MAX_FRAME_SIZE ) ||
		( payload_size < 2 ) || ( payload_size > VIDEO_READ_SIZE ) ||
		( ( unit->Format.Format == VIDEO_FORMAT_YUY2 ) &&
			( (U64) unit->Format.Width * unit->Format.Height * 2 > frame_size ) ) )
	{
		return ( FALSE );
	}

	if ( ! Video_Control( unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x01, 0x0200, probe, size ) )
	{
		return ( FALSE );
	}

	unit->Format.FrameInterval = interval;
	unit->Format.MaxFrameSize = frame_size;
	unit->PayloadSize = payload_size;

	return ( TRUE );
}

// --
