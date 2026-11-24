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
Video_FrameInterval( const U8 *data, U32 length, U32 *number )
{
	U32 interval;
	U32 preferred;
	U32 index;
	U32 count;

	preferred = Video_LE32( & data[21] );
	count = data[25];

	if ( count )
	{
		if ( length < 26 + 4 * count )
		{
			return ( 0 );
		}

		for ( index = 0; index < count; index++ )
		{
			interval = Video_LE32( & data[26 + 4 * index] );

			if ( ( interval ) && ( --*number == 0 ) )
			{
				return ( interval );
			}
		}
	}
	else if ( length >= 38 )
	{
		if ( ( preferred ) && ( --*number == 0 ) )
		{
			return ( preferred );
		}

		// Offer the bounds and the default for a continuous interval range.
		for ( index = 26; index <= 30; index += 4 )
		{
			interval = Video_LE32( & data[index] );

			if ( ( interval ) && ( interval != preferred ) &&
				( ( index == 26 ) || ( interval != Video_LE32( & data[26] ) ) ) &&
				( --*number == 0 ) )
			{
				return ( interval );
			}
		}
	}

	return ( 0 );
}

// --

static S32
Video_ValidInterval( const U8 *data, U32 length, U32 interval )
{
	U32 count;
	U32 index;
	U32 minimum;
	U32 step;

	count = data[25];

	if ( ! interval )
	{
		return ( FALSE );
	}

	if ( ! count )
	{
		if ( length < 38 )
		{
			return ( FALSE );
		}

		minimum = Video_LE32( & data[26] );
		step = Video_LE32( & data[34] );

		return ( ( interval >= minimum ) && ( interval <= Video_LE32( & data[30] ) ) &&
			( ( ! step ) || ( ( interval - minimum ) % step == 0 ) ) );
	}

	if ( length < 26 + 4 * count )
	{
		return ( FALSE );
	}

	for ( index = 0; index < count; index++ )
	{
		if ( Video_LE32( & data[26 + 4 * index] ) == interval )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

SEC_CODE S32
Video_Parse( struct VideoUnit *unit, U32 number, struct VideoMode *mode )
{
struct USB2_Config_Desc *config;

const U8 *data;
U32 remaining;
U32 length;
U32 format;
U32 interval;
U32 wanted;
U8 active;
U8 alt;
U8 index;
U8 defaultFrame;
U8 selectedDefault;

	config = (PTR) unit->StartMsg->ConfigDescriptors;

	if ( ( ! config ) || ( config->Length < sizeof( struct USB2_Config_Desc ) ) ||
		( config->DescriptorType != DSCTYPE_Configuration ) )
	{
		return ( FALSE );
	}

	remaining = LE_SWAP16( config->TotalLength );
	data = (PTR) config;
	format = 0;
	wanted = number;
	mode->Format.Width = 0;
	mode->Format.Format = 0;
	index = 0;
	defaultFrame = 0;
	selectedDefault = 0;
	active = FALSE;
	alt = 0;

	while( remaining >= 2 )
	{
		length = data[0];

		if ( ( length < 2 ) || ( length > remaining ) )
		{
			return ( FALSE );
		}

		if ( ( data[1] == DSCTYPE_Interface ) && ( length >= sizeof( struct USB2_Interface_Desc ) ) )
		{
			active = ( data[2] == unit->InterfaceNr ) && ( data[5] == 0x0e ) && ( data[6] == 0x02 );
			alt = data[3];
		}
		else if ( ( active ) && ( ! alt ) && ( data[1] == 0x24 ) && ( length >= 4 ) )
		{
			if ( ( data[2] == 0x06 ) && ( length >= 11 ) )
			{
				format = VIDEO_FORMAT_MJPEG;
				index = data[3];
				defaultFrame = data[6];
			}
			else if ( ( data[2] == 0x04 ) && ( length >= 22 ) &&
				( data[5] == 'Y' ) && ( data[6] == 'U' ) &&
				( data[7] == 'Y' ) && ( data[8] == '2' ) && ( data[21] == 16 ) )
			{
				format = VIDEO_FORMAT_YUY2;
				index = data[3];
				defaultFrame = length >= 27 ? data[22] : 0;
			}
			else if ( ( data[2] == 0x04 ) || ( data[2] == 0x06 ) || ( data[2] == 0x10 ) )
			{
				format = 0;
			}

			if ( ( index ) && ( length >= 26 ) && ( data[3] ) &&
				( ( ( format == VIDEO_FORMAT_MJPEG ) && ( data[2] == 0x07 ) ) ||
				( ( format == VIDEO_FORMAT_YUY2 ) && ( data[2] == 0x05 ) ) ) &&
				( Video_LE16( & data[5] ) ) && ( Video_LE16( & data[7] ) ) )
			{
				interval = 0;

				if ( number )
				{
					interval = Video_FrameInterval( data, length, & wanted );
				}
				else if ( ( ! mode->Format.Width ) ||
					( ( format == VIDEO_FORMAT_MJPEG ) && ( mode->Format.Format != VIDEO_FORMAT_MJPEG ) ) ||
					( ( index == mode->FormatIndex ) && ( format == mode->Format.Format ) &&
						( ! selectedDefault ) && ( data[3] == defaultFrame ) ) )
				{
					interval = Video_LE32( & data[21] );

					if ( ! Video_ValidInterval( data, length, interval ) )
					{
						wanted = 1;

						if ( data[25] )
						{
							interval = Video_FrameInterval( data, length, & wanted );
						}
						else
						{
							interval = length >= 38 ? Video_LE32( & data[26] ) : 0;
						}
					}
				}

				if ( interval )
				{
					mode->Format.Width = Video_LE16( & data[5] );
					mode->Format.Height = Video_LE16( & data[7] );
					mode->Format.MaxFrameSize = Video_LE32( & data[17] );
					mode->Format.FrameInterval = interval;
					mode->Format.Format = format;
					mode->Format.VendorID = unit->StartMsg->VendorID;
					mode->Format.DeviceID = unit->StartMsg->DeviceID;
					mode->FormatIndex = index;
					mode->FrameIndex = data[3];
					selectedDefault = data[3] == defaultFrame;

					if ( number )
					{
						return ( TRUE );
					}
				}
			}
		}

		data += length;
		remaining -= length;
	}

	return ( ( ! number ) && ( mode->Format.Width ) );
}

// --
