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
#include <exec/execbase.h>
#include <string.h>
#include "../inc/AudioDevice.h"
#include "Wav.h"

// --

#define AUDIO_TEST_WAV_LIMIT ( 32U * 1024U * 1024U )

// --

static void
Audio_CheckFile( const char *name )
{
	BPTR lock;
	char path[512];

	lock = Lock( name, ACCESS_READ );

	if ( ! lock )
	{
		Printf( "%s could not be found or opened (DOS error %ld).\n", name, IoErr() );

		return;
	}

	if ( NameFromLock( lock, path, sizeof( path ) ) )
	{
		Printf( "%s resolves to %s\n", name, path );
	}
	else
	{
		Printf( "%s is present.\n", name );
	}

	UnLock( lock );
}

// --

static void
Audio_CheckSetup( void )
{
	const char *name;
	struct ExecBase *base;
	struct Library *driver;
	struct Node *device;

	char label[96];
	U32 size;
	S32 loaded;
	S32 registered;

	Audio_CheckFile( "LIBS:USB2/cls_Audio.Class" );

	Audio_CheckFile( "LIBS:USB2/cls_Audio.Driver" );

	base = (PTR) IExec->Data.LibBase;
	size = 0;

	Forbid();

	driver = (PTR) FindName( & base->LibList, "cls_Audio.Driver" );

	device = FindName( & base->DeviceList, AUDIO_DEVICE_NAME );

	loaded = driver != NULL;
	registered = device != NULL;

	if ( ( driver ) && ( driver->lib_IdString ) )
	{
		name = driver->lib_IdString;

		while ( ( size < sizeof( label ) - 1 ) && ( name[size] ) &&
			( name[size] != '\r' ) && ( name[size] != '\n' ) )
		{
			label[size] = name[size];
			size++;
		}
	}

	label[size] = 0;

	Permit();

	Printf( "Loaded cls_Audio.Driver: %s\n", size ? label : ( loaded ? "no version string" : "none" ) );

	Printf( "%s registered: %s\n", AUDIO_DEVICE_NAME, registered ? "yes" : "no" );
}

// --

static S32
Audio_Transfer( struct IOStdReq *ioreq, U32 seconds )
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
	timer->Time.Seconds = seconds;
	timer->Time.Microseconds = 0;
	mask = ( 1UL << port->mp_SigBit ) |
		( 1UL << ioreq->io_Message.mn_ReplyPort->mp_SigBit ) | SIGBREAKF_CTRL_C;
	signals = 0;

	SendIO( & timer->Request );

	SendIO( (PTR) ioreq );

	while ( ( ! CheckIO( (PTR) ioreq ) ) && ( ! CheckIO( & timer->Request ) ) &&
		( ! ( signals & SIGBREAKF_CTRL_C ) ) )
	{
		signals = Wait( mask );
	}

	if ( signals & SIGBREAKF_CTRL_C )
	{
		Printf( "Audio transfer interrupted.\n" );
	}
	else if ( ! CheckIO( (PTR) ioreq ) )
	{
		Printf( "Audio transfer timed out.\n" );
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
Audio_PlaySamples( struct IOStdReq *ioreq, const struct AudioFormat *format, U8 *samples, U32 frames )
{
	U32 bytes;
	U32 seconds;

	bytes = frames * format->Channels * 2;
	seconds = 10 + 2 * ( frames / format->SampleRate );
	ioreq->io_Data = samples;
	ioreq->io_Length = bytes;
	ioreq->io_Command = CMD_WRITE;

	if ( ! Audio_Transfer( ioreq, seconds ) )
	{
		return ( RETURN_FAIL );
	}

	if ( ( ioreq->io_Error ) || ( ioreq->io_Actual != bytes ) )
	{
		Printf( "Playback failed: %ld, %lu of %lu bytes sent.\n",
			(S32) ioreq->io_Error, ioreq->io_Actual, bytes );

		if ( ioreq->io_Error == USB2Err_Host_NoBandWidth )
		{
			Printf( "The USB controller could not schedule audio. Stop any other USB capture and retry.\n" );
		}
		else if ( ioreq->io_Error == USB2Err_Host_IllegalSpeed )
		{
			Printf( "The controller cannot schedule audio at this device's USB speed.\n" );

			Printf( "Check the controller and hub path in USB2Analyzer.\n" );
		}

		return ( RETURN_FAIL );
	}

	Printf( "Played %lu bytes.\n", ioreq->io_Actual );

	return ( RETURN_OK );
}

// --

static S32
Audio_PlayTone( struct IOStdReq *ioreq, const struct AudioFormat *format, U8 *samples, U32 frames )
{
	U8 *output;
	U32 frame;
	U32 channel;
	U32 phase;
	S32 sample;

	phase = 0;
	output = samples;

	for( frame = 0; frame < frames; frame++ )
	{
		// Quiet 440 Hz triangle wave. Scale the ramp with the device sample rate.
		sample = ( (U64) phase * 8192 ) / format->SampleRate;
		sample = ( sample < 4096 ? sample : 8192 - sample ) - 2048;
		phase = ( phase + 440 ) % format->SampleRate;

		for( channel = 0; channel < format->Channels; channel++ )
		{
			Audio_WriteLE16( output, (U16) sample );

			output += 2;
		}
	}

	return ( Audio_PlaySamples( ioreq, format, samples, frames ) );
}

// --

static S32
Audio_PlayWav( struct IOStdReq *ioreq, const struct AudioFormat *format, const char *name )
{
	struct AudioWav wav;

	BPTR file;
	S64 size;
	U64 frames;
	U8 *data;
	U8 *samples;
	S32 result;

	result = RETURN_FAIL;
	samples = NULL;

	file = Open( name, MODE_OLDFILE );

	if ( ! file )
	{
		Printf( "Could not open WAV %s.\n", name );

		return ( result );
	}

	size = GetFileSize( file );

	if ( ( size < 12 ) || ( size > AUDIO_TEST_WAV_LIMIT ) )
	{
		Printf( "The WAV must be a readable file of up to 32 MiB.\n" );
		goto close_file;
	}

	data = AllocVecTags( (U32) size, TAG_END );

	if ( ! data )
	{
		Printf( "Could not allocate the WAV buffer.\n" );
		goto close_file;
	}

	if ( ( Read( file, data, (S32) size ) != (S32) size ) ||
		( ! Audio_Wav_Parse( data, (U32) size, & wav ) ) )
	{
		Printf( "Could not read a mono, stereo, or four-channel PCM16 WAV from %s.\n", name );
		goto free_data;
	}

	frames = ( (U64) wav.Frames * format->SampleRate + wav.Rate / 2 ) / wav.Rate;
	frames = MAX( 1, frames );

	if ( frames * format->Channels * 2 > AUDIO_TEST_WAV_LIMIT )
	{
		Printf( "The WAV would exceed the 32 MiB playback buffer after conversion.\n" );
		goto free_data;
	}

	if ( ( wav.Rate != format->SampleRate ) || ( wav.Channels != format->Channels ) )
	{
		samples = AllocVecTags( (U32) frames * format->Channels * 2, TAG_END );

		if ( ! samples )
		{
			Printf( "Could not allocate the converted PCM buffer.\n" );
			goto free_data;
		}

		Audio_Wav_Convert( & wav, samples, (U32) frames, format->SampleRate, format->Channels );
	}

	Printf( "Playing %s (%lu Hz, %lu channel(s))...\n", name, wav.Rate, wav.Channels );

	result = Audio_PlaySamples( ioreq, format, samples ? samples : wav.Samples, (U32) frames );

	FreeVec( samples );

free_data:
	FreeVec( data );

close_file:
	Close( file );

	return ( result );
}

// --

static S32
Audio_List( struct IOStdReq *ioreq )
{
	struct AudioEndpoint *entries;
	struct AudioEndpoint *entry;

	U32 index;
	U32 count;
	U32 rate;
	S32 result;

	entries = AllocVecTags( AUDIO_MAX_UNITS * sizeof( *entries ), TAG_END );

	if ( ! entries )
	{
		return ( RETURN_FAIL );
	}

	result = RETURN_FAIL;
	ioreq->io_Command = AUDIO_CMD_ENUM_ENDPOINTS;
	ioreq->io_Data = entries;
	ioreq->io_Length = AUDIO_MAX_UNITS * sizeof( *entries );
	ioreq->io_Offset = AUDIO_ENDPOINT_VERSION;

	if ( DoIO( (PTR) ioreq ) )
	{
		Printf( "Audio endpoint query failed: %ld\n", (S32) ioreq->io_Error );
	}
	else
	{
		count = ioreq->io_Actual / sizeof( *entries );

		if ( ! count )
		{
			Printf( "No audio endpoints attached.\n" );
		}

		for( index = 0; index < count; index++ )
		{
			entry = & entries[index];

			Printf( "%s  %s  %lu channel(s), %lu-bit PCM  %s\n",
				entry->Direction == AUDIO_ENDPOINT_INPUT ? "Input " : "Output",
				entry->Name, entry->DefaultFormat.Channels, entry->DefaultFormat.BitsPerSample,
				entry->Flags & AUDIO_ENDPOINT_BUSY ? "busy" : "available" );

			Printf( "  ID %s, unit %lu, rates", entry->ID[0] ? entry->ID : "(unavailable)", entry->LegacyUnit );

			if ( entry->RateCount )
			{
				for( rate = 0; rate < entry->RateCount; rate++ )
				{
					Printf( " %lu", entry->Rates[rate] );
				}
			}
			else
			{
				Printf( " %lu-%lu", entry->MinimumRate, entry->MaximumRate );
			}

			Printf( " Hz\n" );
		}

		result = RETURN_OK;
	}

	FreeVec( entries );

	return ( result );
}

// --

int
main( int argc, char **argv )
{
struct MsgPort *port;
struct AudioEndpointRequest *request;
struct IOStdReq *ioreq;
struct AudioFormat format;

U8 header[AUDIO_WAV_MAX_HEADER_SIZE];
U8 *samples;
const char *endpoint;
const char *wav;
U32 unit;
U32 length;
U32 headerBytes;
BPTR file;
S32 result;
S32 playback;
S32 check;
S32 list;
S32 error;

	result = RETURN_FAIL;
	endpoint = NULL;
	wav = NULL;
	playback = FALSE;
	check = FALSE;
	list = FALSE;

	if ( ( ( argc == 2 ) || ( argc == 3 ) ) && ( ! strcmp( argv[1], "PLAY" ) ) )
	{
		playback = TRUE;
		wav = ( argc == 3 ) ? argv[2] : NULL;
	}
	else if ( ( ( argc == 3 ) || ( argc == 4 ) ) && ( ! strcmp( argv[1], "OUTPUT" ) ) )
	{
		playback = TRUE;
		endpoint = argv[2];
		wav = ( argc == 4 ) ? argv[3] : NULL;
	}
	else if ( ( argc == 3 ) && ( ! strcmp( argv[1], "INPUT" ) ) )
	{
		endpoint = argv[2];
	}
	else if ( ( argc == 2 ) && ( ! strcmp( argv[1], "CHECK" ) ) )
	{
		check = TRUE;
	}
	else if ( ( argc == 2 ) && ( ! strcmp( argv[1], "LIST" ) ) )
	{
		list = TRUE;
	}
	else if ( argc > 1 )
	{
		Printf( "Usage: USB2AudioTest [LIST|PLAY [WAV file]|CHECK|INPUT ID|OUTPUT ID [WAV file]]\n" );

		return ( result );
	}

	if ( ( endpoint ) && ( strlen( endpoint ) >= AUDIO_ENDPOINT_ID_SIZE ) )
	{
		Printf( "Audio endpoint ID is too long.\n" );

		return ( result );
	}

	if ( check )
	{
		Audio_CheckSetup();
	}

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		return ( result );
	}

	request = (PTR) AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( *request ),
		ASOIOR_ReplyPort, port,
		TAG_END );

	if ( ! request )
	{
		FreeSysObject( ASOT_PORT, port );

		return ( result );
	}

	ioreq = & request->IO;
	unit = playback ? AUDIO_PLAYBACK_UNIT_BASE : 0;

	if ( list )
	{
		unit = USB2_MEDIA_QUERY_UNIT;
	}
	else if ( endpoint )
	{
		unit = AUDIO_UNIT_ENDPOINT;
		request->Version = AUDIO_ENDPOINT_VERSION;
		request->Direction = playback ? AUDIO_ENDPOINT_OUTPUT : AUDIO_ENDPOINT_INPUT;

		strcpy( request->ID, endpoint );
	}

	error = OpenDevice( AUDIO_DEVICE_NAME, unit, (PTR) ioreq, 0 );

	if ( error )
	{
		Printf( "Could not open %s (error %ld).\n", AUDIO_DEVICE_NAME, error );

		goto cleanup;
	}

	if ( list )
	{
		result = Audio_List( ioreq );

		goto close_device;
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
		( ( format.Channels != 1 ) && ( format.Channels != 2 ) &&
			( ( playback ) || ( format.Channels != 4 ) ) ) )
	{
		Printf( "The device did not report a supported PCM format.\n" );
		goto close_device;
	}

	Printf( "USB %04lx:%04lx, %ld Hz, %ld channel(s), %ld-bit PCM\n",
		format.VendorID, format.DeviceID, format.SampleRate,
		format.Channels, format.BitsPerSample );

	if ( check )
	{
		result = RETURN_OK;
		goto close_device;
	}

	if ( wav )
	{
		result = Audio_PlayWav( ioreq, & format, wav );
		goto close_device;
	}

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

	Printf( "Recording one second from the USB microphone...\n" );

	if ( ! Audio_Transfer( ioreq, 10 ) )
	{
		goto free_samples;
	}

	if ( ioreq->io_Error == AUDIO_ERR_SHORT_READ )
	{
		Printf( "Some USB audio intervals were lost. Saving the PCM received.\n" );
	}
	else if ( ioreq->io_Error )
	{
		Printf( "Capture failed: %ld, %ld bytes received\n",
			(S32) ioreq->io_Error, ioreq->io_Actual );

		if ( ioreq->io_Error == USB2Err_Host_NoBandWidth )
		{
			Printf( "The USB controller could not schedule the microphone. Close the video preview and retry.\n" );
		}
		else if ( ioreq->io_Error == AUDIO_ERR_NO_SAMPLES )
		{
			Printf( "The microphone returned no PCM samples in consecutive USB reads.\n" );
		}
		else if ( ioreq->io_Error == AUDIO_ERR_RATE_SETUP )
		{
			Printf( "The microphone did not accept the sample-rate setup request.\n" );
		}
		else if ( ioreq->io_Error == AUDIO_ERR_NO_PACKETS )
		{
			Printf( "Every isochronous packet failed in consecutive USB reads.\n" );
		}

		goto free_samples;
	}

	if ( ( ! ioreq->io_Actual ) || ( ioreq->io_Actual > length ) ||
		( ioreq->io_Actual % ( format.Channels * 2 ) ) )
	{
		Printf( "Capture returned an invalid PCM length (%lu bytes).\n", ioreq->io_Actual );
		goto free_samples;
	}

	length = ioreq->io_Actual;

	headerBytes = Audio_Wav_Header( header, length, format.SampleRate, format.Channels );

	file = Open( "RAM:usb2audio.wav", MODE_NEWFILE );

	if ( ! file )
	{
		Printf( "Could not create RAM:usb2audio.wav\n" );
		goto free_samples;
	}

	if ( ( Write( file, header, headerBytes ) == (S32) headerBytes ) &&
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
