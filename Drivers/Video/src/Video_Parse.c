/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static U16
Video_LE16( const U8 *data )
{
	return ( (U16) data[0] | ( (U16) data[1] << 8 ) );
}

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
			}
			else if ( ( data[2] == 0x04 ) && ( length >= 22 ) &&
				( data[5] == 'Y' ) && ( data[6] == 'U' ) &&
				( data[7] == 'Y' ) && ( data[8] == '2' ) && ( data[21] == 16 ) )
			{
				format = VIDEO_FORMAT_YUY2;
				index = data[3];
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
					( ( format == VIDEO_FORMAT_MJPEG ) && ( mode->Format.Format != VIDEO_FORMAT_MJPEG ) ) )
				{
					interval = Video_LE32( & data[21] );
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

					if ( number )
					{
						return ( TRUE );
					}
				}
			}
		}
		else if ( ( active ) && ( ! alt ) && ( data[1] == DSCTYPE_EndPoint ) &&
			( length >= sizeof( struct USB2_EndPoint_Desc ) ) &&
			( ( data[2] & EPADR_Dir_Mask ) == EPADR_Dir_In ) &&
			( ( data[3] & EPATT_Type_Mask ) == EPATT_Type_Bulk ) )
		{
			if ( ! number )
			{
				unit->BulkEndPoint = data[2];
			}
		}

		data += length;
		remaining -= length;
	}

	return ( ( ! number ) && ( mode->Format.Width ) );
}

// --
