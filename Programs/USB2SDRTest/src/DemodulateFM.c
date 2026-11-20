
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <math.h>
#include <string.h>
#include <proto/exec.h>
#include "SDRTest.h"
#include "../../../Drivers/Audio/Test/Wav.h"

// --

#define FM_PI		3.14159265358979323846
#define FM_IQ_TAPS	161U
#define FM_AUDIO_TAPS	193U
#define FM_FILTER_SCALE	32768.0
#define FM_PHASE_SCALE	10000.0
#define FM_PHASES	16U
#define FM_IQ_HISTORY	( FM_IQ_TAPS - 1 )
#define FM_AUDIO_HISTORY ( FM_AUDIO_TAPS - 1 )

struct SDRFMDecoder
{
	U8 IQ[( SDR_FM_BLOCK_PAIRS + FM_IQ_HISTORY ) * 2];
	S16 MPX[SDR_FM_BLOCK_PAIRS / SDR_FM_DECIMATION + FM_AUDIO_HISTORY];
	S16 IQCoefficients[FM_IQ_TAPS];
	S16 AudioCoefficients[FM_PHASES][FM_AUDIO_TAPS];
	double OffsetI;
	double OffsetQ;
	double PreviousI;
	double PreviousQ;
	double PreviousAudio;
	double DCRemoved;
	double Deemphasized;
	double Alpha;
	double DCDecay;
	U32 Rate;
	U32 Position;
	S32 Started;
};

// --

static void
BuildLowpass( S16 *coefficients, U32 taps, double cutoff, double phase )
{
	double kernel[FM_AUDIO_TAPS];
	double total;
	double position;
	double window;
	double angle;
	U32 tap;

	total = 0.0;

	for( tap = 0; tap < taps; tap++ )
	{
		position = (double) tap - ( taps - 1 ) * 0.5 - phase;
		angle = 2.0 * FM_PI * tap / ( taps - 1 );

		window = 0.42 - 0.5 * cos( angle ) + 0.08 * cos( 2.0 * angle );

		kernel[tap] = window * ( position ? sin( 2.0 * FM_PI * cutoff * position ) / ( FM_PI * position ) : 2.0 * cutoff );

		total += kernel[tap];
	}

	for( tap = 0; tap < taps; tap++ )
	{
		coefficients[tap] = lround( kernel[tap] * FM_FILTER_SCALE / total );
	}
}

// --

struct SDRFMDecoder *
SDR_CreateFMDecoder( U32 audioRate )
{
	struct SDRFMDecoder *decoder;

	U32 phase;
	double cutoff;

	if ( ( ! audioRate ) || ( audioRate > SDR_FM_MPX_RATE ) )
	{
		return ( NULL );
	}

	decoder = AllocVecTags( sizeof( *decoder ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! decoder )
	{
		return ( NULL );
	}

	decoder->Rate = audioRate;
	decoder->Position = SDR_FM_MPX_RATE - 1;
	decoder->Alpha = 1.0 - exp( -1.0 / ( audioRate * 50e-6 ) );
	decoder->DCDecay = exp( -2.0 * FM_PI * 20.0 / audioRate );
	cutoff = MIN( 15000.0, audioRate * 0.4 ) / SDR_FM_MPX_RATE;

	BuildLowpass( decoder->IQCoefficients, FM_IQ_TAPS, 106000.0 / SDR_SAMPLE_RATE, 0.0 );

	for( phase = 0; phase < FM_PHASES; phase++ )
	{
		BuildLowpass( decoder->AudioCoefficients[phase], FM_AUDIO_TAPS, cutoff, (double) phase / FM_PHASES );
	}

	return ( decoder );
}

// --

static void
Demodulate( struct SDRFMDecoder *decoder, const U8 *iq, U32 samples )
{
	const U8 *sample;
	const U8 *back;
	S16 *mpx;
	U32 index;
	U32 tap;
	U32 pairs;
	S32 i;
	S32 q;
	double centeredI;
	double centeredQ;
	double cross;
	double dot;

	pairs = samples * SDR_FM_DECIMATION;
	mpx = decoder->MPX + FM_AUDIO_HISTORY;

	if ( ! decoder->Started )
	{
		for( tap = 0; tap < FM_IQ_HISTORY; tap++ )
		{
			decoder->IQ[tap * 2] = iq[0];
			decoder->IQ[tap * 2 + 1] = iq[1];
		}
	}

	memcpy( decoder->IQ + FM_IQ_HISTORY * 2, iq, pairs * 2 );

	// The symmetric channel filter uses one coefficient for both ends.
	for( index = 0; index < samples; index++ )
	{
		sample = decoder->IQ + index * SDR_FM_DECIMATION * 2;
		back = sample + ( FM_IQ_TAPS - 1 ) * 2;
		i = ( sample[FM_IQ_HISTORY] - 128 ) * decoder->IQCoefficients[FM_IQ_TAPS / 2];
		q = ( sample[FM_IQ_HISTORY + 1] - 128 ) * decoder->IQCoefficients[FM_IQ_TAPS / 2];

		for( tap = 0; tap < FM_IQ_TAPS / 2; tap++, sample += 2, back -= 2 )
		{
			i += ( sample[0] + back[0] - 256 ) * decoder->IQCoefficients[tap];
			q += ( sample[1] + back[1] - 256 ) * decoder->IQCoefficients[tap];
		}

		if ( ! decoder->Started )
		{
			decoder->OffsetI = i;
			decoder->OffsetQ = q;
			decoder->Started = TRUE;
		}

		decoder->OffsetI += 0.00245 * ( i - decoder->OffsetI );
		decoder->OffsetQ += 0.00245 * ( q - decoder->OffsetQ );
		centeredI = i - decoder->OffsetI;
		centeredQ = q - decoder->OffsetQ;
		cross = decoder->PreviousI * centeredQ - decoder->PreviousQ * centeredI;
		dot = decoder->PreviousI * centeredI + decoder->PreviousQ * centeredQ;
		decoder->PreviousI = centeredI;
		decoder->PreviousQ = centeredQ;

		*mpx++ = ( ( cross ) || ( dot ) ) ? lround( atan2( cross, dot ) * FM_PHASE_SCALE ) : 0;
	}

	memmove( decoder->IQ, decoder->IQ + pairs * 2, FM_IQ_HISTORY * 2 );
}

// --

U32
SDR_ProcessFM( struct SDRFMDecoder *decoder, const U8 *iq, U32 pairs, U8 *pcm )
{
	const S16 *coefficients;
	const S16 *sample;
	U32 samples;
	U32 position;
	U32 limit;
	U32 phase;
	U32 tap;
	U32 frames;
	S64 sum;
	S32 value;
	double audio;
	double scale;

	samples = pairs / SDR_FM_DECIMATION;
	position = decoder->Position;
	limit = samples * decoder->Rate;
	frames = 0;
	scale = 28000.0 * SDR_FM_MPX_RATE / ( 2.0 * FM_PI * 75000.0 * FM_PHASE_SCALE * FM_FILTER_SCALE );

	if ( ! samples )
	{
		return ( 0 );
	}

	Demodulate( decoder, iq, samples );

	// Keep the fractional source position across blocks at the output device's rate.
	while( position < limit )
	{
		phase = ( position % decoder->Rate ) * FM_PHASES / decoder->Rate;
		coefficients = decoder->AudioCoefficients[phase];
		sample = decoder->MPX + position / decoder->Rate;
		sum = 0;

		for( tap = 0; tap < FM_AUDIO_TAPS; tap++ )
		{
			sum += sample[tap] * coefficients[tap];
		}

		audio = sum * scale;
		decoder->DCRemoved = audio - decoder->PreviousAudio + decoder->DCDecay * decoder->DCRemoved;
		decoder->PreviousAudio = audio;
		decoder->Deemphasized += decoder->Alpha * ( decoder->DCRemoved - decoder->Deemphasized );

		value = lround( decoder->Deemphasized );

		Audio_WriteLE16( pcm, (U16) MIN( 32767, MAX( -32768, value ) ) );

		pcm += 2;
		position += SDR_FM_MPX_RATE;
		frames++;
	}

	decoder->Position = position - limit;

	memmove( decoder->MPX, decoder->MPX + samples, FM_AUDIO_HISTORY * sizeof( *sample ) );

	return ( frames );
}

// --
