
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <devices/usb2.h>
#include <string.h>
#include "Wav.h"

// --

#define WAV_PCM 0x0001
#define WAV_EXTENSIBLE 0xfffe
#define WAV_PCM_FORMAT_SIZE 16
#define WAV_EXTENSIBLE_FORMAT_SIZE 40
#define WAV_EXTENSIBLE_EXTRA_SIZE 22

// KSDATAFORMAT_SUBTYPE_PCM in RIFF byte order.
static const U8 PCMSubtype[16] =
{
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
	0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
};

// --

void
Audio_WriteLE16( U8 *data, U32 value )
{
	data[0] = value;
	data[1] = value >> 8;
}

static void
Audio_WriteLE32( U8 *data, U32 value )
{
	data[0] = value;
	data[1] = value >> 8;
	data[2] = value >> 16;
	data[3] = value >> 24;
}

// --

U32
Audio_Wav_HeaderSize( U32 channels )
{
	return ( channels == 4 ? AUDIO_WAV_MAX_HEADER_SIZE : 44 );
}

// --

U32
Audio_Wav_Header( U8 *header, U32 length, U32 rate, U32 channels )
{
	U8 *data;
	U32 size;
	S32 extensible;

	size = Audio_Wav_HeaderSize( channels );

	extensible = ( channels == 4 );
	data = header + size - 8;

	memcpy( header, "RIFF", 4 );

	Audio_WriteLE32( & header[4], size - 8 + length );

	memcpy( header + 8, "WAVEfmt ", 8 );

	Audio_WriteLE32( & header[16], extensible ? WAV_EXTENSIBLE_FORMAT_SIZE : WAV_PCM_FORMAT_SIZE );

	Audio_WriteLE16( & header[20], extensible ? WAV_EXTENSIBLE : WAV_PCM );

	Audio_WriteLE16( & header[22], channels );

	Audio_WriteLE32( & header[24], rate );

	Audio_WriteLE32( & header[28], rate * channels * 2 );

	Audio_WriteLE16( & header[32], channels * 2 );

	Audio_WriteLE16( & header[34], 16 );

	if ( extensible )
	{
		Audio_WriteLE16( & header[36], WAV_EXTENSIBLE_EXTRA_SIZE );

		Audio_WriteLE16( & header[38], 16 );

		// The microphone array has no assigned speaker positions.
		Audio_WriteLE32( & header[40], 0 );

		memcpy( header + 44, PCMSubtype, sizeof( PCMSubtype ) );
	}

	memcpy( data, "data", 4 );

	Audio_WriteLE32( data + 4, length );

	return ( size );
}

// --

static U32
ReadLE16( const U8 *data )
{
	return ( data[0] | ( (U32) data[1] << 8 ) );
}

// --

static U32
ReadLE32( const U8 *data )
{
	return ( data[0] | ( (U32) data[1] << 8 ) | ( (U32) data[2] << 16 ) | ( (U32) data[3] << 24 ) );
}

// --

static S32
IsPCM16( const U8 *format, U32 size )
{
	U32 tag;
	U32 extra;

	if ( ( size < WAV_PCM_FORMAT_SIZE ) || ( ReadLE16( format + 14 ) != 16 ) )
	{
		return ( FALSE );
	}

	tag = ReadLE16( format );

	if ( tag == WAV_PCM )
	{
		return ( TRUE );
	}

	if ( ( tag != WAV_EXTENSIBLE ) || ( size < WAV_EXTENSIBLE_FORMAT_SIZE ) )
	{
		return ( FALSE );
	}

	extra = ReadLE16( format + 16 );

	return ( ( extra >= WAV_EXTENSIBLE_EXTRA_SIZE ) && ( extra <= size - 18 ) &&
		( ReadLE16( format + 18 ) == 16 ) && ( ! memcmp( format + 24, PCMSubtype, sizeof( PCMSubtype ) ) ) );
}

// --

S32
Audio_Wav_Parse( U8 *data, U32 length, struct AudioWav *wav )
{
	U32 end;
	U32 offset;
	U32 size;
	U32 pcmLength;
	U32 frameSize;
	U8 *chunk;

	if ( ( length < 12 ) || ( memcmp( data, "RIFF", 4 ) ) || ( memcmp( data + 8, "WAVE", 4 ) ) )
	{
		return ( FALSE );
	}

	size = ReadLE32( data + 4 );

	if ( ( size < 4 ) || ( size > length - 8 ) )
	{
		return ( FALSE );
	}

	end = size + 8;
	pcmLength = 0;
	frameSize = 0;
	wav->Samples = NULL;

	for ( offset = 12; end - offset >= 8; offset += size + ( size & 1 ) )
	{
		chunk = data + offset;

		size = ReadLE32( chunk + 4 );

		offset += 8;

		if ( ( size > end - offset ) || ( ( size & 1 ) && ( size == end - offset ) ) )
		{
			return ( FALSE );
		}

		if ( ( ! frameSize ) && ( ! memcmp( chunk, "fmt ", 4 ) ) )
		{
			if ( ! IsPCM16( chunk + 8, size ) )
			{
				return ( FALSE );
			}

			wav->Channels = ReadLE16( chunk + 10 );

			wav->Rate = ReadLE32( chunk + 12 );

			frameSize = ReadLE16( chunk + 20 );

			if ( ( ( wav->Channels != 1 ) && ( wav->Channels != 2 ) && ( wav->Channels != 4 ) ) ||
				( frameSize != wav->Channels * 2 ) || ( ! wav->Rate ) || ( wav->Rate > 0x00ffffff ) )
			{
				return ( FALSE );
			}
		}
		else if ( ( ! wav->Samples ) && ( ! memcmp( chunk, "data", 4 ) ) )
		{
			wav->Samples = data + offset;
			pcmLength = size;
		}

		if ( ( frameSize ) && ( wav->Samples ) )
		{
			if ( ( ! pcmLength ) || ( pcmLength % frameSize ) )
			{
				return ( FALSE );
			}

			wav->Frames = pcmLength / frameSize;

			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

static S32
SampleForChannel( const struct AudioWav *wav, U32 frame, U32 channel, U32 channels )
{
	const U8 *sample;

	S32 left;
	S32 right;

	sample = wav->Samples + frame * wav->Channels * 2;

	left = (S16) ReadLE16( sample );

	if ( wav->Channels == 4 )
	{
		// Monitor the microphone array without assigning its channels to speakers.
		return ( ( left + (S16) ReadLE16( sample + 2 ) +
			(S16) ReadLE16( sample + 4 ) + (S16) ReadLE16( sample + 6 ) ) / 4 );
	}

	if ( wav->Channels == 1 )
	{
		return ( left );
	}

	right = (S16) ReadLE16( sample + 2 );

	if ( channels == 1 )
	{
		return ( ( left + right ) / 2 );
	}

	return ( channel ? right : left );
}

// --

void
Audio_Wav_Convert( const struct AudioWav *wav, U8 *output, U32 frames, U32 rate, U32 channels )
{
	U64 position;
	U64 step;
	U32 frame;
	U32 source;
	U32 next;
	U32 channel;
	U32 sourceChannels;
	S32 fraction;
	S32 first;
	S32 second;
	S32 sample;

	if ( ( rate == wav->Rate ) && ( channels == wav->Channels ) && ( frames <= wav->Frames ) )
	{
		memcpy( output, wav->Samples, frames * channels * 2 );

		return;
	}

	position = 0;
	step = ( ( (U64) wav->Rate << 32 ) + rate / 2 ) / rate;
	sourceChannels = MIN( wav->Channels, channels );

	for ( frame = 0; frame < frames; frame++ )
	{
		source = MIN( position >> 32, wav->Frames - 1 );
		next = MIN( source + 1, wav->Frames - 1 );
		// Fifteen fractional bits keep interpolation of signed PCM in 32 bits.
		fraction = (U32) position >> 17;

		for ( channel = 0; channel < sourceChannels; channel++ )
		{
			first = SampleForChannel( wav, source, channel, channels );

			second = SampleForChannel( wav, next, channel, channels );

			sample = first + ( ( second - first ) * fraction ) / 32768;
			output[0] = (U16) sample;
			output[1] = (U16) sample >> 8;
			output += 2;
		}

		if ( sourceChannels < channels )
		{
			output[0] = output[-2];
			output[1] = output[-1];
			output += 2;
		}

		position += step;
	}
}

// --
U32
Audio_Wav_ConvertBlock( struct AudioWavResampler *state, const struct AudioWav *wav,
	U8 *output, U32 rate, U32 channels )
{
	S32 next[2];
	S32 fraction;
	S32 sample;
	U32 input;
	U32 written;
	U32 channel;

	if ( ( ! wav->Rate ) || ( ! rate ) || ( ! channels ) || ( channels > 2 ) )
	{
		return ( 0 );
	}

	if ( wav->Rate == rate )
	{
		Audio_Wav_Convert( wav, output, wav->Frames, rate, channels );

		return ( wav->Frames );
	}

	written = 0;

	for ( input = 0; input < wav->Frames; input++ )
	{
		for ( channel = 0; channel < channels; channel++ )
		{
			next[channel] = SampleForChannel( wav, input, channel, channels );
		}

		if ( state->Ready )
		{
			// Keep the next output position in units of 1/rate input intervals.
			while ( state->Phase < rate )
			{
				fraction = ( state->Phase * 32768 ) / rate;

				for ( channel = 0; channel < channels; channel++ )
				{
					sample = state->Previous[channel] +
						( ( next[channel] - state->Previous[channel] ) * fraction ) / 32768;

					Audio_WriteLE16( output, sample );

					output += 2;
				}

				state->Phase += wav->Rate;
				written++;
			}

			state->Phase -= rate;
		}

		for ( channel = 0; channel < channels; channel++ )
		{
			state->Previous[channel] = next[channel];
		}

		state->Ready = TRUE;
	}

	return ( written );
}

// --
