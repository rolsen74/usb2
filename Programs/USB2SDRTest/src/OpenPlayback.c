
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <exec/io.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "SDRTest.h"
#include "Playback.h"

// --

S32
SDR_OpenPlayback( struct SDRPlayback *play, U32 output )
{
	struct IOStdReq *request;

	U32 unit;
	U32 last;
	U32 frames;
	S32 error;

	play->Port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! play->Port )
	{
		goto allocation_failed;
	}

	request = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *request ),
		ASOIOR_ReplyPort, play->Port, TAG_END );

	if ( ! request )
	{
		goto allocation_failed;
	}

	play->Request = request;
	unit = ( output == SDR_OUTPUT_AUTO ) ? AUDIO_PLAYBACK_UNIT_BASE : output;
	last = ( output == SDR_OUTPUT_AUTO ) ? AUDIO_MAX_UNITS - 1 : output;

	for( ; unit <= last; unit++ )
	{
		error = OpenDevice( AUDIO_DEVICE_NAME, unit, (PTR) request, 0 );

		if ( error )
		{
			continue;
		}

		request->io_Command = AUDIO_CMD_QUERY;
		request->io_Data = & play->Format;
		request->io_Length = sizeof( play->Format );

		error = DoIO( (PTR) request );

		if ( ( ! error ) && ( request->io_Actual == sizeof( play->Format ) ) &&
			( play->Format.Encoding == AUDIO_ENCODING_PCM_S16LE ) &&
			( play->Format.BitsPerSample == 16 ) &&
			( play->Format.Channels >= 1 ) && ( play->Format.Channels <= 2 ) &&
			( play->Format.SampleRate ) && ( play->Format.SampleRate <= SDR_FM_MPX_RATE ) )
		{
			play->Opened = TRUE;
			play->Unit = unit;

			break;
		}

		CloseDevice( (PTR) request );
	}

	if ( ! play->Opened )
	{
		Printf( "No compatible USB audio playback unit found in %lu-%lu.\n",
			( output == SDR_OUTPUT_AUTO ) ? (U32) AUDIO_PLAYBACK_UNIT_BASE : output, last );

		return ( FALSE );
	}

	frames = ( ( SDR_FM_BLOCK_PAIRS / SDR_FM_DECIMATION ) * play->Format.SampleRate +
		SDR_FM_MPX_RATE - 1 ) / SDR_FM_MPX_RATE;
	play->BlockBytes = frames * play->Format.Channels * 2;

	play->Staging = AllocVecTags( play->BlockBytes * SDR_PLAYBACK_BLOCKS, TAG_END );

	play->Playing = AllocVecTags( play->BlockBytes * SDR_PLAYBACK_BLOCKS, TAG_END );

	if ( ( ! play->Staging ) || ( ! play->Playing ) )
	{
		goto allocation_failed;
	}

	return ( TRUE );

allocation_failed:
	Printf( "Could not allocate the USB audio playback resources.\n" );

	return ( FALSE );
}

// --
