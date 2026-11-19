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
Audio_Rate24( const U8 *data )
{
	return ( (U32) data[0] | ( (U32) data[1] << 8 ) | ( (U32) data[2] << 16 ) );
}

static U32
Audio_SelectRate( const U8 *format, U32 capacity, U32 interval, U8 frequency )
{
	U32 limit;
	U32 rate;
	U32 low;
	U32 high;
	U32 count;
	U32 index;

	count = format[7];
	limit = (U64) ( capacity / ( format[4] * 2 ) ) * 1000000U / interval;

	if ( ! count )
	{
		low = Audio_Rate24( format + 8 );

		high = Audio_Rate24( format + 11 );

		high = MIN( high, limit );

		if ( ( ! frequency ) || ( ! low ) || ( low > high ) )
		{
			return ( 0 );
		}

		rate = MAX( low, MIN( high, 48000U ) );

		return ( ( (U64) rate * interval >= 1000000U ) ? rate : 0 );
	}

	if ( ( count > 1 ) && ( ! frequency ) )
	{
		return ( 0 );
	}

	for( index = 0; index < count; index++ )
	{
		rate = Audio_Rate24( format + 8 + 3 * index );

		if ( ( rate ) && ( rate <= limit ) && ( (U64) rate * interval >= 1000000U ) )
		{
			return ( rate );
		}
	}

	return ( 0 );
}

// --

SEC_CODE S32
Audio_Rate_Valid( struct AudioUnit *unit, U32 sample_rate )
{
	const U8 *format;

	U64 samples;
	U32 count;
	U32 index;

	format = unit->RateDescriptor;
	count = format[7];
	samples = (U64) sample_rate * unit->IntervalUS;

	if ( ( samples < 1000000U ) ||
		( samples > (U64) ( unit->PacketBytes / ( unit->Format.Channels * 2 ) ) * 1000000U ) )
	{
		return ( FALSE );
	}

	if ( ! count )
	{
		return ( ( sample_rate >= Audio_Rate24( format + 8 ) ) &&
			( sample_rate <= Audio_Rate24( format + 11 ) ) );
	}

	for( index = 0; index < count; index++ )
	{
		if ( sample_rate == Audio_Rate24( format + 8 + 3 * index ) )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

SEC_CODE S32
Audio_Parse( struct AudioUnit *unit )
{
struct USB2_Config_Desc *config;

const U8 *data;
const U8 *format;
U32 remaining;
U32 config_size;
U32 length;
U32 rate;
U32 packet;
U32 speed;
U32 interval;
U32 transactions;
U8 endpoint;
U8 alt;
U8 active;
U8 pcm;
U8 frequency;


	config = (PTR) unit->StartMsg->ConfigDescriptors;

	if ( ( ! config ) || ( ! unit->StartMsg->Function ) )
	{
		return ( FALSE );
	}

	if ( USB2_Attr_Get( unit->StartMsg->Function->FKT_ID, USB2Tag_Attr_ConfigSize,
		& config_size, sizeof( config_size ) ) != sizeof( config_size ) )
	{
		AUDIO_TRACE( "could not query configuration descriptor size\n" );

		return ( FALSE );
	}

	if ( ( config_size < sizeof( *config ) ) || ( config->Length < sizeof( *config ) ) ||
		( config->DescriptorType != DSCTYPE_Configuration ) ||
		( LE_SWAP16( config->TotalLength ) != config_size ) )
	{
		AUDIO_TRACE( "invalid configuration descriptor, reported %lu bytes\n", config_size );

		return ( FALSE );
	}

	if ( ( USB2_Attr_Get( unit->StartMsg->Function->FKT_ID, USB2Tag_Attr_Speed,
		& speed, sizeof( speed ) ) != sizeof( speed ) ) ||
		( ( speed != USBSPEED_Full ) && ( speed != USBSPEED_High ) ) )
	{
		AUDIO_TRACE( "could not query a supported USB speed\n" );

		return ( FALSE );
	}

	remaining = config_size;
	data = (PTR) config;
	rate = 0;
	packet = 0;
	interval = 0;
	format = NULL;
	endpoint = 0;
	alt = 0;
	active = FALSE;
	pcm = FALSE;
	frequency = FALSE;

	while( remaining )
	{
		if ( remaining < 2 )
		{
			return ( FALSE );
		}

		length = data[0];

		if ( ( length < 2 ) || ( length > remaining ) )
		{
			return ( FALSE );
		}

		if ( ( data[1] == DSCTYPE_Interface ) && ( length >= sizeof( struct USB2_Interface_Desc ) ) )
		{
			if ( ( active ) && ( pcm ) && ( format ) && ( endpoint ) )
			{
				rate = Audio_SelectRate( format, packet, interval, frequency );

				if ( rate )
				{
					break;
				}
			}

			active = ( data[2] == unit->InterfaceNr ) && ( data[3] ) &&
				( data[5] == USBCLASS_Audio ) && ( data[6] == 0x02 ) && ( data[7] == 0 );
			alt = data[3];

			if ( active )
			{
				AUDIO_TRACE( "checking interface %lu alt %lu, %lu endpoint(s)\n",
					(U32) unit->InterfaceNr, (U32) alt, (U32) data[4] );
			}

			packet = 0;
			interval = 0;
			format = NULL;
			endpoint = 0;
			pcm = FALSE;
			frequency = FALSE;
		}
		else if ( ( active ) && ( data[1] == 0x24 ) && ( length >= 4 ) )
		{
			if ( ( data[2] == 0x01 ) && ( length >= 7 ) )
			{
				pcm = ( data[5] == 1 ) && ( data[6] == 0 );
			}
			else if ( ( data[2] == 0x02 ) && ( length >= 11 ) &&
				( data[3] == 1 ) && ( data[4] >= 1 ) && ( data[4] <= 2 ) &&
				( data[5] == 2 ) && ( data[6] == 16 ) &&
				( length >= 8U + 3U * ( data[7] ? data[7] : 2 ) ) )
			{
				format = data;
			}
		}
		else if ( ( active ) && ( data[1] == DSCTYPE_EndPoint ) )
		{
			endpoint = 0;
			frequency = FALSE;

			if ( ( length >= 9 ) && ( data[2] & 0x0f ) && ( data[6] >= 1 ) && ( data[6] <= 8 ) &&
				( ( data[3] & 0x33 ) == EPATT_Type_Isochronous ) &&
				( ( data[2] & EPADR_Dir_Mask ) ||
				  ( ( ( data[3] & 0x0c ) >= 0x08 ) && ( ! data[8] ) ) ) )
			{
				packet = (U32) data[4] | ( (U32) data[5] << 8 );
				transactions = ( packet >> 11 ) & 3;
				packet &= 0x7ff;

				if ( ( packet ) && ( transactions <= 2 ) &&
					( ( ( speed == USBSPEED_High ) && ( packet <= 1024 ) ) ||
					  ( ( speed == USBSPEED_Full ) && ( packet <= 1023 ) && ( ! transactions ) ) ) )
				{
					endpoint = data[2];
					packet *= transactions + 1;
					interval = ( speed == USBSPEED_High ? 125U : 1000U ) << ( data[6] - 1 );
				}
			}
		}
		else if ( ( active ) && ( endpoint ) && ( data[1] == 0x25 ) && ( length >= 4 ) && ( data[2] == 0x01 ) )
		{
			frequency = data[3] & 1;
		}

		data += length;
		remaining -= length;
	}

	if ( ( ! active ) || ( ! pcm ) || ( ! format ) || ( ! endpoint ) )
	{
		return ( FALSE );
	}

	if ( ! rate )
	{
		rate = Audio_SelectRate( format, packet, interval, frequency );
	}

	if ( ! rate )
	{
		return ( FALSE );
	}

	unit->AltNr = alt;
	unit->EndPoint = endpoint;
	unit->PacketBytes = packet;
	unit->IntervalUS = interval;
	unit->PacketCount = MAX( 1U, MIN( AUDIO_PACKET_COUNT, AUDIO_BATCH_US / interval ) );
	unit->FrequencyControl = frequency;
	unit->RateDescriptor = format;
	unit->Format.Encoding = AUDIO_ENCODING_PCM_S16LE;
	unit->Format.SampleRate = rate;
	unit->Format.Channels = format[4];
	unit->Format.BitsPerSample = 16;
	unit->Format.PacketBytes = packet;
	unit->Format.VendorID = unit->StartMsg->VendorID;
	unit->Format.DeviceID = unit->StartMsg->DeviceID;

	return ( TRUE );
}

// --
