
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

#define SDR_LIVE_BYTES	( SDR_FM_BLOCK_PAIRS * 2 )
#define SDR_LIVE_READS	2U
#define SDR_LIVE_SPARE	SDR_LIVE_READS

// --

static S32
ReceiverDetached( struct USB2_Register *reg )
{
	struct USB2_NotifyMessage *msg;

	S32 detached;

	detached = FALSE;

	while(( msg = (PTR) GetMsg( reg->Stack_MsgPort ) ))
	{
		if ( msg->nm_Command == USBNC_Detached )
		{
			detached = TRUE;
		}

		ReplyMsg( (PTR) msg );
	}

	return ( detached );
}

// --

static S32
Listen( struct USB2_Register *reg, struct USB2_EPResource *bulk, struct SDRFMDecoder *decoder,
	struct SDRPlayback *play, U8 **iq, U8 *pcm )
{
	struct USB2_IORequest *request;

	U8 *samples;
	U32 slot;
	U32 frames;
	U32 signals;
	U32 waitMask;
	S32 error;
	S32 result;

	waitMask = bulk->MsgPortBit | reg->Stack_MsgPortBit | ( 1UL << play->Port->mp_SigBit ) | SIGBREAKF_CTRL_C;
	result = RETURN_FAIL;

	for( slot = 0; slot < SDR_LIVE_READS; slot++ )
	{
		request = bulk->IORequests[slot];
		request->io_Command = CMD_READ;
		request->io_Data = iq[slot];
		request->io_Length = SDR_LIVE_BYTES;
		request->io_TimeOut = 2000000;
		request->io_AllowShortPackets = FALSE;

		SendIO( (PTR) request );
	}

	slot = 0;

	if ( SDR_HoldFIFO( reg->Res_Control, FALSE ) )
	{
		goto bailout;
	}

	for( ;; )
	{
		if ( CheckSignal( SIGBREAKF_CTRL_C ) )
		{
			result = RETURN_OK;

			break;
		}

		if ( ReceiverDetached( reg ) )
		{
			Printf( "The SDR receiver was disconnected.\n" );

			break;
		}

		if ( ! SDR_ServicePlayback( play ) )
		{
			break;
		}

		request = bulk->IORequests[slot];

		if ( ! CheckIO( (PTR) request ) )
		{
			signals = Wait( waitMask );

			if ( signals & SIGBREAKF_CTRL_C )
			{
				result = RETURN_OK;

				break;
			}

			continue;
		}

		error = WaitIO( (PTR) request );

		request->io_Data = NULL;

		if ( ( error ) || ( request->io_Actual != SDR_LIVE_BYTES ) )
		{
			Printf( "SDR bulk read failed (USB error %ld, %lu of %lu bytes).\n",
				error, request->io_Actual, (U32) SDR_LIVE_BYTES );

			break;
		}

		// Requeue immediately using the spare buffer while decoding this block.
		samples = iq[slot];
		iq[slot] = iq[SDR_LIVE_SPARE];
		iq[SDR_LIVE_SPARE] = samples;
		request->io_Data = iq[slot];

		SendIO( (PTR) request );

		slot = ( slot + 1 ) % SDR_LIVE_READS;

		frames = SDR_ProcessFM( decoder, samples, SDR_FM_BLOCK_PAIRS, pcm );

		if ( ! SDR_QueuePlayback( play, pcm, frames ) )
		{
			break;
		}
	}

bailout:
	for( slot = 0; slot < SDR_LIVE_READS; slot++ )
	{
		request = bulk->IORequests[slot];

		if ( ( request->io_Data ) && ( ! CheckIO( (PTR) request ) ) )
		{
			AbortIO( (PTR) request );
		}
	}

	for( slot = 0; slot < SDR_LIVE_READS; slot++ )
	{
		request = bulk->IORequests[slot];

		if ( request->io_Data )
		{
			WaitIO( (PTR) request );

			request->io_Data = NULL;
		}
	}

	return ( result );
}

// --

S32
SDR_ListenFM( struct USB2_Register *reg, U8 endpoint, U32 frequency, U32 output )
{
	struct SDRPlayback play;
	struct SDRFMDecoder *decoder;
	struct USB2_EPResource *bulk;

	U8 *iq[SDR_LIVE_READS + 1];
	U8 *pcm;
	U32 slot;
	S32 result;

	play = (struct SDRPlayback) { 0 };
	decoder = NULL;
	pcm = NULL;
	result = RETURN_FAIL;

	for( slot = 0; slot <= SDR_LIVE_SPARE; slot++ )
	{
		iq[slot] = NULL;
	}

	if ( ! SDR_OpenPlayback( & play, output ) )
	{
		goto bailout;
	}

	decoder = SDR_CreateFMDecoder( play.Format.SampleRate );

	pcm = AllocVecTags( play.BlockBytes / play.Format.Channels, TAG_END );

	if ( ( ! decoder ) || ( ! pcm ) )
	{
		goto allocation_failed;
	}

	for( slot = 0; slot <= SDR_LIVE_SPARE; slot++ )
	{
		iq[slot] = AllocVecTags( SDR_LIVE_BYTES, TAG_END );

		if ( ! iq[slot] )
		{
			goto allocation_failed;
		}
	}

	bulk = SDR_OpenIQ( reg, endpoint, frequency, SDR_LIVE_READS );

	if ( ! bulk )
	{
		goto bailout;
	}

	Printf( "Listening at %lu Hz on USB audio unit %lu (%lu Hz PCM16, %lu channel(s)). Ctrl+C stops.\n",
		frequency, play.Unit, play.Format.SampleRate, play.Format.Channels );

	result = Listen( reg, bulk, decoder, & play, iq, pcm );

	if ( ( ! SDR_CloseIQ( reg->Res_Control, bulk ) ) && ( result == RETURN_OK ) )
	{
		result = RETURN_WARN;
	}

	if ( ( play.ShortBytes ) || ( play.SkippedBlocks ) )
	{
		Printf( "FM playback lost %lu bytes to USB short writes and skipped %lu audio blocks.\n",
			play.ShortBytes, play.SkippedBlocks );

		if ( result == RETURN_OK )
		{
			result = RETURN_WARN;
		}
	}

	goto bailout;

allocation_failed:
	Printf( "Could not allocate the live FM buffers.\n" );

bailout:
	SDR_ClosePlayback( & play );

	for( slot = 0; slot <= SDR_LIVE_SPARE; slot++ )
	{
		FreeVec( iq[slot] );
	}

	FreeVec( pcm );

	FreeVec( decoder );

	return ( result );
}

// --
