
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
	U32 frame;
	U32 source;
	U32 next;
	U32 fraction;
	U32 channel;
	S32 first;
	S32 second;
	S32 sample;

	position = 0;

	for ( frame = 0; frame < frames; frame++ )
	{
		source = MIN( position / rate, wav->Frames - 1 );
		next = MIN( source + 1, wav->Frames - 1 );
		fraction = position % rate;

		for ( channel = 0; channel < channels; channel++ )
		{
			first = SampleForChannel( wav, source, channel, channels );

			second = SampleForChannel( wav, next, channel, channels );

			sample = first + ( (S64) ( second - first ) * fraction ) / rate;
			output[0] = (U16) sample;
			output[1] = (U16) sample >> 8;
			output += 2;
		}

		position += wav->Rate;
	}
}

// --
