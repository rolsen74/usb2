/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

#define AUDIO_SPLIT_PAYLOAD 188U

static U32
Audio_Rate24( const U8 *data )
{
	return ( (U32) data[0] | ( (U32) data[1] << 8 ) | ( (U32) data[2] << 16 ) );
}

static U32
Audio_SelectRate( const U8 *format, U32 capacity, U32 interval, U8 frequency, S32 highest )
{
	U32 limit;
	U32 rate;
	U32 low;
	U32 high;
	U32 count;
	U32 index;
	U32 selected;

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

	selected = 0;

	for( index = 0; index < count; index++ )
	{
		rate = Audio_Rate24( format + 8 + 3 * index );

		if ( ( rate ) && ( rate <= limit ) && ( (U64) rate * interval >= 1000000U ) )
		{
			if ( ! highest )
			{
				return ( rate );
			}

			selected = MAX( selected, rate );
		}
	}

	return ( selected );
}

// --

SEC_CODE S32
Audio_Rate_Valid( struct AudioUnit *unit, U32 sample_rate )
{
	const U8 *format;

	U64 samples;
	U32 count;
	U32 index;

	if ( unit->Protocol == AUDIO_PROTOCOL_UAC2 )
	{
		return ( sample_rate == unit->Format.SampleRate );
	}

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

static U32
Audio_ProbeClock( struct AudioUnit *unit, U32 config_size, U8 terminal, U8 endpoint )
{
	U32 index;
	U16 key;

	if ( ! Audio_FindClock( unit, config_size, terminal, endpoint ) )
	{
		return ( 0 );
	}

	key = ( (U16) unit->ClockInterface << 8 ) | unit->ClockEntity;

	for( index = 0; index < unit->ClockProbes; index++ )
	{
		if ( unit->ClockKeys[index] == key )
		{
			return ( unit->ClockRates[index] );
		}
	}

	if ( unit->ClockProbes == AUDIO_CLOCK_PROBE_LIMIT )
	{
		return ( 0 );
	}

	unit->ClockKeys[index] = key;
	unit->ClockProbes++;

	unit->ClockRates[index] = Audio_ClockRate( unit );

	return ( unit->ClockRates[index] );
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
U32 candidate_packet;
U32 speed;
U32 hcd;
U32 interval;
U32 transactions;
S32 uac2;
S32 ehci;
S32 ohci;
U8 endpoint;
U8 descriptor_endpoint;
U8 alt;
U8 active;
U8 pcm;
U8 frequency;
U8 terminal;
U8 channels;
U8 endpoints;


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

	hcd = HCDTYPE_Last;

	USB2_Attr_Get( unit->StartMsg->Function->FKT_ID, USB2Tag_Attr_HCDType, & hcd, sizeof( hcd ) );

	ehci = ( hcd == HCDTYPE_EHCI ) || ( hcd == HCDTYPE_P5020 ) ||
		( hcd == HCDTYPE_P5040 ) || ( hcd == HCDTYPE_TABOR ) ||
		( ( hcd == HCDTYPE_SAM460 ) && ( speed == USBSPEED_High ) );
	ohci = ( hcd == HCDTYPE_OHCI ) || ( hcd == HCDTYPE_SAM440 ) || ( hcd == HCDTYPE_SAM460 );
	data = unit->ParseNext ? unit->ParseNext : (PTR) config;
	remaining = config_size - ( data - (const U8 *) config );
	rate = 0;
	packet = 0;
	interval = 0;
	format = NULL;
	endpoint = 0;
	descriptor_endpoint = 0;
	alt = 0;
	active = FALSE;
	pcm = FALSE;
	frequency = FALSE;
	terminal = 0;
	channels = 0;
	endpoints = 0;
	uac2 = ( unit->Protocol == AUDIO_PROTOCOL_UAC2 );

	for( ;; )
	{
		length = 0;

		if ( remaining )
		{
			length = data[0];

			if ( ( length < 2 ) || ( length > remaining ) )
			{
				return ( FALSE );
			}
		}

		if ( ( ! remaining ) || ( ( data[1] == DSCTYPE_Interface ) &&
			( length >= sizeof( struct USB2_Interface_Desc ) ) ) )
		{
			if ( ( active ) && ( pcm ) && ( format ) && ( endpoint ) && ( channels ) &&
				( ( channels <= 2 ) || ( endpoint & EPADR_Dir_Mask ) ) )
			{
				if ( uac2 )
				{
					rate = Audio_ProbeClock( unit, config_size, terminal, endpoint );

					if ( ( (U64) rate * interval < 1000000U ) ||
						( (U64) rate * interval > (U64) ( packet / ( channels * 2 ) ) * 1000000U ) )
					{
						rate = 0;
					}
				}
				else
				{
					rate = Audio_SelectRate( format, packet, interval, frequency, FALSE );
				}

				if ( rate )
				{
					unit->ParseNext = data;

					break;
				}
			}

			if ( ! remaining )
			{
				break;
			}

			active = ( data[2] == unit->InterfaceNr ) && ( data[3] ) &&
				( data[5] == USBCLASS_Audio ) && ( data[6] == 0x02 ) && ( data[7] == unit->Protocol );
			alt = data[3];
			endpoints = data[4];

			if ( active )
			{
				AUDIO_TRACE( "checking interface %lu alt %lu, %lu endpoint(s)\n",
					(U32) unit->InterfaceNr, (U32) alt, (U32) data[4] );
			}

			packet = 0;
			interval = 0;
			format = NULL;
			endpoint = 0;
			descriptor_endpoint = 0;
			pcm = FALSE;
			frequency = FALSE;
			terminal = 0;
			channels = 0;
		}
		else if ( ( active ) && ( data[1] == 0x24 ) && ( length >= 4 ) )
		{
			if ( uac2 )
			{
				if ( ( data[2] == 0x01 ) && ( length >= 16 ) )
				{
					terminal = data[3];
					channels = data[10];
					pcm = ( data[5] == 1 ) && ( data[6] == 1 ) && ( ! data[7] ) &&
						( ! data[8] ) && ( ! data[9] ) && ( channels >= 1 ) && ( channels <= 2 );
				}
				else if ( ( data[2] == 0x02 ) && ( length >= 6 ) &&
					( data[3] == 1 ) && ( data[4] == 2 ) && ( data[5] == 16 ) )
				{
					format = data;
				}
			}
			else if ( ( data[2] == 0x01 ) && ( length >= 7 ) )
			{
				pcm = ( data[5] == 1 ) && ( data[6] == 0 );
				terminal = data[3];
			}
			else if ( ( data[2] == 0x02 ) && ( length >= 11 ) &&
				( data[3] == 1 ) && ( data[4] >= 1 ) &&
				( ( data[4] <= 2 ) || ( ( data[4] == 4 ) &&
					( unit->StartMsg->VendorID == 0x1415 ) && ( unit->StartMsg->DeviceID == 0x2000 ) ) ) &&
				( data[5] == 2 ) && ( data[6] == 16 ) &&
				( length >= 8U + 3U * ( data[7] ? data[7] : 2 ) ) )
			{
				format = data;
				channels = data[4];
			}
		}
		else if ( ( active ) && ( data[1] == DSCTYPE_EndPoint ) )
		{
			S32 fixed_packet;
			S32 no_feedback;
			U8 sync;

			descriptor_endpoint = ( length >= 3 ) ? data[2] : 0;

			if ( ( ! endpoint ) && ( length >= ( uac2 ? 7U : 9U ) ) &&
				( data[2] & 0x0f ) && ( data[6] >= 1 ) && ( data[6] <= 8 ) &&
				( ( data[3] & 0x33 ) == EPATT_Type_Isochronous ) )
			{
				sync = data[3] & 0x0c;
				no_feedback = uac2 ? ( endpoints == 1 ) : ( ! data[8] );
				candidate_packet = (U32) data[4] | ( (U32) data[5] << 8 );
				transactions = ( candidate_packet >> 11 ) & 3;
				candidate_packet &= 0x7ff;

				// Some full-speed outputs omit the sync type and use fixed-size packets.
				fixed_packet = FALSE;

				if ( ( ! uac2 ) && ( ! sync ) && ( speed == USBSPEED_Full ) &&
					( data[6] == 1 ) && ( format ) && ( format[7] == 1 ) )
				{
					fixed_packet = ( Audio_Rate24( format + 8 ) * format[4] * 2 == candidate_packet * 1000U );
				}

				if ( ( candidate_packet ) && ( transactions <= 2 ) &&
					( ( ( speed == USBSPEED_High ) && ( candidate_packet <= 1024 ) ) ||
					  ( ( speed == USBSPEED_Full ) && ( candidate_packet <= 1023 ) && ( ! transactions ) ) ) &&
					( ( data[2] & EPADR_Dir_Mask ) ||
					  ( ( no_feedback ) && ( ( sync >= 0x08 ) || ( fixed_packet ) ) ) ) )
				{
					endpoint = data[2];
					packet = candidate_packet * ( transactions + 1 );
					interval = ( speed == USBSPEED_High ? 125U : 1000U ) << ( data[6] - 1 );
				}
			}
		}
		else if ( ( ! uac2 ) && ( active ) && ( endpoint ) && ( descriptor_endpoint == endpoint ) &&
			( data[1] == 0x25 ) && ( length >= 4 ) && ( data[2] == 0x01 ) )
		{
			frequency = data[3] & 1;
		}

		data += length;
		remaining -= length;
	}

	if ( ! rate )
	{
		return ( FALSE );
	}

	unit->AltNr = alt;
	unit->EndPoint = endpoint;
	unit->PacketBytes = packet;
	unit->IntervalUS = interval;
	unit->PacketCount = MAX( 1U, MIN( ( hcd == HCDTYPE_XHCI ) ? AUDIO_XHCI_PACKET_COUNT : AUDIO_PACKET_COUNT,
		( ( endpoint & EPADR_Dir_Mask ) ? AUDIO_BATCH_US : AUDIO_WRITE_BATCH_US ) / interval ) );
	unit->ReadSlots = ( ohci || ehci ) ? AUDIO_STREAM_REQUESTS : 1;
	unit->SingleSplitRate = 0;
	unit->GainEntity = 0;
	unit->GainValid = FALSE;
	unit->FrequencyControl = frequency;
	unit->RateDescriptor = uac2 ? NULL : format;
	unit->Format.Encoding = AUDIO_ENCODING_PCM_S16LE;
	unit->Format.SampleRate = rate;
	unit->Format.Channels = channels;
	unit->Format.BitsPerSample = 16;
	unit->Format.PacketBytes = packet;
	unit->Format.VendorID = unit->StartMsg->VendorID;
	unit->Format.DeviceID = unit->StartMsg->DeviceID;

	unit->TerminalLink = terminal;

	if ( ( ! uac2 ) && ( endpoint & EPADR_Dir_Mask ) )
	{
		Audio_FindGain( unit, config_size );
	}
	else if ( ( ! uac2 ) && ( ehci ) && ( speed == USBSPEED_Full ) && ( frequency ) &&
		( (U64) rate * interval > (U64) ( AUDIO_SPLIT_PAYLOAD / ( channels * 2 ) ) * 1000000U ) )
	{
		// A single start-split avoids needing a second microframe to deliver the packet.
		unit->SingleSplitRate = Audio_SelectRate( format, MIN( packet, AUDIO_SPLIT_PAYLOAD ),
			interval, frequency, TRUE );
	}

	return ( TRUE );
}

// --
