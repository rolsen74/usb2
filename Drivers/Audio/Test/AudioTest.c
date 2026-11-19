/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/timer.h>
#include <devices/usb2.h>
#include <string.h>
#include "../inc/AudioDevice.h"

// --

static void
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

static S32
Audio_Capture( struct IOStdReq *ioreq )
{
	struct MsgPort *port;
	struct TimeRequest *timer;

	U32 mask;
	U32 signals;
	S32 result;

	result = FALSE;

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		Printf( "Could not allocate timer port.\n" );

		return ( FALSE );
	}

	timer = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *timer ),
		ASOIOR_ReplyPort, port, TAG_END );

	if ( ! timer )
	{
		Printf( "Could not allocate timer request.\n" );
		goto free_port;
	}

	if ( OpenDevice( TIMERNAME, UNIT_MICROHZ, & timer->Request, 0 ) )
	{
		Printf( "Could not open timer.device.\n" );
		goto free_timer;
	}

	timer->Request.io_Command = TR_ADDREQUEST;
	timer->Time.Seconds = 10;
	timer->Time.Microseconds = 0;
	mask = ( 1UL << port->mp_SigBit ) |
		( 1UL << ioreq->io_Message.mn_ReplyPort->mp_SigBit ) | SIGBREAKF_CTRL_C;
	signals = 0;

	Printf( "Recording one second from the USB microphone...\n" );

	SendIO( & timer->Request );

	SendIO( (PTR) ioreq );

	while ( ( ! CheckIO( (PTR) ioreq ) ) && ( ! CheckIO( & timer->Request ) ) &&
		( ! ( signals & SIGBREAKF_CTRL_C ) ) )
	{
		signals = Wait( mask );
	}

	if ( signals & SIGBREAKF_CTRL_C )
	{
		Printf( "Capture interrupted.\n" );
	}
	else if ( ! CheckIO( (PTR) ioreq ) )
	{
		Printf( "Audio capture timed out.\n" );
	}
	else
	{
		result = TRUE;
	}

	if ( ! CheckIO( (PTR) ioreq ) )
	{
		AbortIO( (PTR) ioreq );
	}

	WaitIO( (PTR) ioreq );

	if ( ! CheckIO( & timer->Request ) )
	{
		AbortIO( & timer->Request );
	}

	WaitIO( & timer->Request );

	CloseDevice( & timer->Request );

free_timer:
	FreeSysObject( ASOT_IOREQUEST, timer );

free_port:
	FreeSysObject( ASOT_PORT, port );

	return ( result );
}

// --

static U32
Audio_Peak( const U8 *samples, U32 length )
{
	U32 index;
	U32 peak;
	S32 sample;

	peak = 0;

	for ( index = 0; index + 1 < length; index += 2 )
	{
		sample = (S16) ( samples[index] | ( (U16) samples[index + 1] << 8 ) );

		if ( sample < 0 )
		{
			sample = -sample;
		}

		peak = MAX( peak, (U32) sample );
	}

	return ( peak );
}

// --

static S32
Audio_PlayTone( struct IOStdReq *ioreq, const struct AudioFormat *format, U8 *samples, U32 frames )
{
	U32 frame;
	U32 channel;
	U32 phase;
	S32 sample;

	phase = 0;
	ioreq->io_Data = samples;
	ioreq->io_Length = frames * format->Channels * 2;
	ioreq->io_Command = CMD_WRITE;

	for( frame = 0; frame < frames; frame++ )
	{
		// Quiet 440 Hz triangle wave. Scale the ramp with the device sample rate.
		sample = ( (U64) phase * 8192 ) / format->SampleRate;
		sample = ( sample < 4096 ? sample : 8192 - sample ) - 2048;
		phase = ( phase + 440 ) % format->SampleRate;

		for( channel = 0; channel < format->Channels; channel++ )
		{
			Audio_WriteLE16( samples, (U16) sample );

			samples += 2;
		}
	}

	if ( DoIO( (PTR) ioreq ) )
	{
		Printf( "Playback failed: %ld, %ld bytes sent\n", (S32) ioreq->io_Error, ioreq->io_Actual );

		return ( RETURN_FAIL );
	}

	Printf( "Played %ld bytes\n", ioreq->io_Actual );

	return ( RETURN_OK );
}

// --

int
main( int argc, char **argv )
{
struct MsgPort *port;
struct IOStdReq *ioreq;
struct AudioFormat format;

U8 header[44];
U8 *samples;
U32 length;
BPTR file;
S32 result;
S32 playback;

	result = RETURN_FAIL;
	playback = ( argc == 2 ) && ( ! strcmp( argv[1], "PLAY" ) );

	if ( ( argc > 1 ) && ( ! playback ) )
	{
		Printf( "Usage: USB2AudioTest [PLAY]\n" );

		return ( result );
	}

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		return ( result );
	}

	ioreq = (PTR) AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( struct IOStdReq ),
		ASOIOR_ReplyPort, port,
		TAG_END );

	if ( ! ioreq )
	{
		FreeSysObject( ASOT_PORT, port );

		return ( result );
	}

	if ( OpenDevice( AUDIO_DEVICE_NAME, playback ? AUDIO_PLAYBACK_UNIT_BASE : 0, (PTR) ioreq, 0 ) )
	{
		Printf( "No supported USB audio device is available on unit %ld.\n",
			(S32) ( playback ? AUDIO_PLAYBACK_UNIT_BASE : 0 ) );
		goto cleanup;
	}

	ioreq->io_Command = AUDIO_CMD_QUERY;
	ioreq->io_Data = & format;
	ioreq->io_Length = sizeof( format );

	if ( DoIO( (PTR) ioreq ) )
	{
		Printf( "Audio format query failed: %ld\n", (S32) ioreq->io_Error );
		goto close_device;
	}

	if ( ( format.Encoding != AUDIO_ENCODING_PCM_S16LE ) || ( format.BitsPerSample != 16 ) ||
		( ! format.SampleRate ) || ( format.SampleRate > 0x00ffffff ) ||
		( ( format.Channels != 1 ) && ( format.Channels != 2 ) ) )
	{
		Printf( "The device did not report a supported PCM format.\n" );
		goto close_device;
	}

	Printf( "USB %04lx:%04lx, %ld Hz, %ld channel(s), %ld-bit PCM\n",
		format.VendorID, format.DeviceID, format.SampleRate,
		format.Channels, format.BitsPerSample );

	length = format.SampleRate * format.Channels * 2;
	samples = AllocVecTags( length, TAG_END );

	if ( ! samples )
	{
		Printf( "Could not allocate sample buffer.\n" );
		goto close_device;
	}

	if ( playback )
	{
		result = Audio_PlayTone( ioreq, & format, samples, format.SampleRate );

		goto free_samples;
	}

	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = samples;
	ioreq->io_Length = length;

	if ( ! Audio_Capture( ioreq ) )
	{
		goto free_samples;
	}

	if ( ioreq->io_Error )
	{
		Printf( "Capture failed: %ld, %ld bytes received\n",
			(S32) ioreq->io_Error, ioreq->io_Actual );
		goto free_samples;
	}

	if ( ( ! ioreq->io_Actual ) || ( ioreq->io_Actual > length ) ||
		( ioreq->io_Actual % ( format.Channels * 2 ) ) )
	{
		Printf( "Capture returned an invalid PCM length (%lu bytes).\n", ioreq->io_Actual );
		goto free_samples;
	}

	length = ioreq->io_Actual;
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
	Audio_WriteLE16( & header[22], format.Channels );
	Audio_WriteLE32( & header[24], format.SampleRate );
	Audio_WriteLE32( & header[28], format.SampleRate * format.Channels * 2 );
	Audio_WriteLE16( & header[32], format.Channels * 2 );
	Audio_WriteLE16( & header[34], 16 );
	header[36] = 'd';
	header[37] = 'a';
	header[38] = 't';
	header[39] = 'a';
	Audio_WriteLE32( & header[40], length );

	file = Open( "RAM:usb2audio.wav", MODE_NEWFILE );

	if ( ! file )
	{
		Printf( "Could not create RAM:usb2audio.wav\n" );
		goto free_samples;
	}

	if ( ( Write( file, header, sizeof( header ) ) == sizeof( header ) ) &&
		( Write( file, samples, length ) == (S32) length ) )
	{
		Printf( "Saved %ld bytes to RAM:usb2audio.wav, peak %lu/32768\n",
			length, Audio_Peak( samples, length ) );
		result = RETURN_OK;
	}
	else
	{
		Printf( "Could not write complete WAV file.\n" );
	}

	Close( file );

free_samples:
	FreeVec( samples );

close_device:
	CloseDevice( (PTR) ioreq );

cleanup:
	FreeSysObject( ASOT_IOREQUEST, ioreq );
	FreeSysObject( ASOT_PORT, port );

	return ( result );
}

// --
