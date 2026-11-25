
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include "SDRTest.h"
#include "../../../Drivers/Audio/Test/Wav.h"

// --

static S32
WriteFM( const char *path, const U8 *pcm, U32 frames )
{
	U8 header[AUDIO_WAV_MAX_HEADER_SIZE];
	U32 headerBytes;
	BPTR file;
	S32 written;
	S32 closed;

	headerBytes = Audio_Wav_Header( header, frames * 2, SDR_FM_AUDIO_RATE, 1 );

	file = Open( path, MODE_NEWFILE );

	if ( ! file )
	{
		PrintFault( IoErr(), path );

		return ( FALSE );
	}

	written = ( ( Write( file, header, headerBytes ) == (S32) headerBytes ) &&
		( Write( file, pcm, frames * 2 ) == (S32) ( frames * 2 ) ) );

	closed = Close( file );

	if ( ( ! written ) || ( ! closed ) )
	{
		Printf( "Could not save the complete FM audio to %s.\n", path );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

S32
SDR_SaveFM( const char *path, const U8 *iq, U32 bytes )
{
	struct SDRFMDecoder *decoder;

	U8 *pcm;
	U32 pairs;
	U32 block;
	U32 frames;
	U32 processed;
	S32 saved;

	pairs = bytes / 2;
	frames = ( (U64) ( pairs / SDR_FM_DECIMATION ) * SDR_FM_AUDIO_RATE ) / SDR_FM_MPX_RATE;
	processed = 0;
	saved = FALSE;

	if ( ! frames )
	{
		Printf( "The I/Q data is too short to decode.\n" );

		return ( FALSE );
	}

	decoder = SDR_CreateFMDecoder( SDR_FM_AUDIO_RATE );

	pcm = AllocVecTags( frames * 2, TAG_END );

	if ( ( ! decoder ) || ( ! pcm ) )
	{
		Printf( "Could not allocate the FM decode buffers.\n" );

		goto bailout;
	}

	while( pairs )
	{
		if ( CheckSignal( SIGBREAKF_CTRL_C ) )
		{
			Printf( "FM decoding interrupted.\n" );

			goto bailout;
		}

		block = MIN( pairs, SDR_FM_BLOCK_PAIRS );

		processed += SDR_ProcessFM( decoder, iq, block, pcm + processed * 2 );

		iq += block * 2;
		pairs -= block;
	}

	saved = WriteFM( path, pcm, processed );

	if ( saved )
	{
		Printf( "Saved %lu mono PCM16 frames at %lu Hz to %s.\n",
			(unsigned long) processed, (unsigned long) SDR_FM_AUDIO_RATE, path );
	}

bailout:
	FreeVec( pcm );

	FreeVec( decoder );

	return ( saved );
}

// --
