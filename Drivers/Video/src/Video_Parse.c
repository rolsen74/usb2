/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static U16 Video_LE16( const U8 *data )
{
	return ( (U16) data[0] | ( (U16) data[1] << 8 ) );
}

// --

SEC_CODE S32 Video_Parse( struct VideoUnit *unit )
{
struct USB2_Config_Desc *config;

const U8 *data;
U32 remaining;
U32 length;
U32 format;
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

			if ( ( ! unit->Format.Width ) && ( format ) && ( length >= 26 ) &&
				( ( ( format == VIDEO_FORMAT_MJPEG ) && ( data[2] == 0x07 ) ) ||
				( ( format == VIDEO_FORMAT_YUY2 ) && ( data[2] == 0x05 ) ) ) )
			{
				unit->Format.Width = Video_LE16( & data[5] );
				unit->Format.Height = Video_LE16( & data[7] );
				unit->Format.MaxFrameSize = Video_LE32( & data[17] );
				unit->Format.FrameInterval = Video_LE32( & data[21] );
				unit->Format.Format = format;
				unit->FormatIndex = index;
				unit->FrameIndex = data[3];
			}
		}
		else if ( ( active ) && ( ! alt ) && ( data[1] == DSCTYPE_EndPoint ) &&
			( length >= sizeof( struct USB2_EndPoint_Desc ) ) &&
			( ( data[2] & EPADR_Dir_Mask ) == EPADR_Dir_In ) &&
			( ( data[3] & EPATT_Type_Mask ) == EPATT_Type_Bulk ) )
		{
			unit->EndPoint = data[2];
		}

		data += length;
		remaining -= length;
	}

	unit->Format.VendorID = unit->StartMsg->VendorID;
	unit->Format.DeviceID = unit->StartMsg->DeviceID;

	return ( ( ( unit->EndPoint ) || ( Video_Select_Stream( unit, 2 ) ) ) && ( unit->Format.Width ) &&
		( unit->Format.Height ) &&
		( unit->Format.FrameInterval ) && ( unit->FormatIndex ) && ( unit->FrameIndex ) );
}

// --
