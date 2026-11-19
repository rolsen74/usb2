
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

void
Audio_Wav_Header( U8 *header, U32 length, U32 rate, U32 channels )
{
	header[0] = 'R';
	header[1] = 'I';
	header[2] = 'F';
	header[3] = 'F';
	Audio_WriteLE32( & header[4], 36 + length );
	header[8] = 'W';
	header[9] = 'A';
	header[10] = 'V';
	header[11] = 'E';
	header[12] = 'f';
	header[13] = 'm';
	header[14] = 't';
	header[15] = ' ';
	Audio_WriteLE32( & header[16], 16 );
	Audio_WriteLE16( & header[20], 1 );
	Audio_WriteLE16( & header[22], channels );
	Audio_WriteLE32( & header[24], rate );
	Audio_WriteLE32( & header[28], rate * channels * 2 );
	Audio_WriteLE16( & header[32], channels * 2 );
	Audio_WriteLE16( & header[34], 16 );
	header[36] = 'd';
	header[37] = 'a';
	header[38] = 't';
	header[39] = 'a';
	Audio_WriteLE32( & header[40], length );
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
			if ( ( size < 16 ) || ( ReadLE16( chunk + 8 ) != 1 ) || ( ReadLE16( chunk + 22 ) != 16 ) )
			{
				return ( FALSE );
			}

			wav->Channels = ReadLE16( chunk + 10 );

			wav->Rate = ReadLE32( chunk + 12 );

			frameSize = ReadLE16( chunk + 20 );

			if ( ( ( wav->Channels != 1 ) && ( wav->Channels != 2 ) ) ||
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
