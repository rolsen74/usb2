
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
SDR_ServicePlayback( struct SDRPlayback *play )
{
	struct IOStdReq *request;

	U8 *buffer;
	S32 error;

	request = play->Request;

	if ( play->Pending )
	{
		if ( ! CheckIO( (PTR) request ) )
		{
			return ( TRUE );
		}

		error = WaitIO( (PTR) request );

		play->Pending = FALSE;
		play->DroppedSinceReply = 0;

		if ( ( error == AUDIO_ERR_SHORT_WRITE ) && ( request->io_Actual ) &&
			( request->io_Actual < request->io_Length ) )
		{
			play->ShortBytes += request->io_Length - request->io_Actual;
		}
		else if ( ( error ) || ( request->io_Actual != request->io_Length ) )
		{
			Printf( "FM playback failed (error %ld, %lu of %lu bytes).\n",
				error, request->io_Actual, request->io_Length );

			return ( FALSE );
		}
	}

	if ( play->StagedBlocks == SDR_PLAYBACK_BLOCKS )
	{
		// usb2audio accepts one public write at a time.
		buffer = play->Playing;
		play->Playing = play->Staging;
		play->Staging = buffer;
		request->io_Command = CMD_WRITE;
		request->io_Data = play->Playing;
		request->io_Length = play->StagedBytes;
		play->StagedBytes = 0;
		play->StagedBlocks = 0;
		play->Pending = TRUE;

		SendIO( (PTR) request );
	}

	return ( TRUE );
}

// --

S32
SDR_QueuePlayback( struct SDRPlayback *play, const U8 *pcm, U32 frames )
{
	U8 *dest;
	U32 frame;

	if ( ! frames )
	{
		return ( TRUE );
	}

	if ( play->StagedBlocks == SDR_PLAYBACK_BLOCKS )
	{
		play->SkippedBlocks++;
		play->DroppedSinceReply++;

		if ( play->DroppedSinceReply >= 32 )
		{
			Printf( "USB audio playback stalled.\n" );

			return ( FALSE );
		}

		return ( TRUE );
	}

	dest = play->Staging + play->StagedBytes;

	if ( play->Format.Channels == 2 )
	{
		for( frame = 0; frame < frames; frame++, pcm += 2, dest += 4 )
		{
			dest[0] = pcm[0];
			dest[1] = pcm[1];
			dest[2] = pcm[0];
			dest[3] = pcm[1];
		}
	}
	else
	{
		CopyMem( (PTR) pcm, dest, frames * 2 );
	}

	play->StagedBytes += frames * play->Format.Channels * 2;
	play->StagedBlocks++;

	return ( SDR_ServicePlayback( play ) );
}

// --
