
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <exec/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/ahi.h>
#include <devices/usb2.h>
#include "PreviewAudio.h"

// --

#define PREVIEW_AUDIO_CHUNK_US 32000U
#define PREVIEW_USB_WRITE_BLOCKS 2
#define PREVIEW_AUDIO_AHI ~0U

// --

static struct IOStdReq *
AudioRequest( struct MsgPort *port )
{
	return ( (PTR) AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( struct IOStdReq ),
		ASOIOR_ReplyPort, port, TAG_END ) );
}

// --

static S32
QueryAudioFormat( struct IOStdReq *request, struct AudioFormat *format )
{
	request->io_Command = AUDIO_CMD_QUERY;
	request->io_Data = format;
	request->io_Length = sizeof( *format );

	if ( DoIO( (PTR) request ) )
	{
		return ( FALSE );
	}

	return ( ( request->io_Actual == sizeof( *format ) ) &&
		( format->Encoding == AUDIO_ENCODING_PCM_S16LE ) && ( format->BitsPerSample == 16 ) &&
		( ( format->Channels == 1 ) || ( format->Channels == 2 ) ) &&
		( format->SampleRate ) && ( format->SampleRate <= 0x00ffffff ) );
}

// --

static void
ReadInputGain( struct IOStdReq *request, struct PreviewAudioChoices *inputs, U32 index )
{
	struct AudioGain gain;

	S32 current;

	request->io_Command = AUDIO_CMD_GET_GAIN;
	request->io_Data = & gain;
	request->io_Length = sizeof( gain );

	if ( ( DoIO( (PTR) request ) ) || ( request->io_Actual != sizeof( gain ) ) ||
		( gain.Minimum < -32768 ) || ( gain.Maximum > 32767 ) ||
		( gain.Minimum >= gain.Maximum ) || ( gain.Step <= 0 ) ||
		( gain.Step > gain.Maximum - gain.Minimum ) )
	{
		return;
	}

	current = MIN( gain.Maximum, MAX( gain.Minimum, gain.Current ) );
	inputs->Gains[index] = gain;
	inputs->GainLevels[index] = ( current - gain.Minimum ) * 100 / ( gain.Maximum - gain.Minimum );
}

// --

static S32
AudioRateIO( struct IOStdReq *request, U16 command, U32 rate )
{
	S32 error;

	request->io_Command = command;
	request->io_Data = &rate;
	request->io_Length = sizeof( rate );

	error = DoIO( (PTR) request );

	request->io_Data = NULL;

	return ( error );
}

// --

static void
ReadOutputRates( struct IOStdReq *request, struct PreviewAudioRates *rates )
{
	static const U32 values[] = { 44100, 48000 };
	static STR const labels[] = { "44100 Hz", "48000 Hz" };

	U32 index;

	for ( index = 0; index < sizeof( values ) / sizeof( values[0] ); index++ )
	{
		if ( ! AudioRateIO( request, AUDIO_CMD_CHECK_RATE, values[index] ) )
		{
			rates->Values[rates->Count] = values[index];
			rates->Labels[rates->Count] = labels[index];
			rates->Count++;
		}
	}
}

// --

static struct IOStdReq *
OpenAHI( struct MsgPort *port )
{
	struct AHIRequest *request;

	request = (PTR) AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *request ),
		ASOIOR_ReplyPort, port, TAG_END );

	if ( ! request )
	{
		return ( NULL );
	}

	request->ahir_Version = 4;

	if ( OpenDevice( AHINAME, AHI_DEFAULT_UNIT, (PTR) request, 0 ) )
	{
		FreeSysObject( ASOT_IOREQUEST, request );

		return ( NULL );
	}

	return ( & request->ahir_Std );
}

// --

void
PreviewAudio_SelectInput( struct PreviewAudioChoices *inputs, U32 vendor, U32 product )
{
	U32 index;
	U32 id;

	id = ( vendor << 16 ) | product;

	for ( index = 0; index < inputs->Count; index++ )
	{
		if ( inputs->USBIDs[index] == id )
		{
			inputs->Selected = index;

			return;
		}
	}
}

// --

void
PreviewAudio_Find( struct PreviewAudioChoices *inputs, struct PreviewAudioChoices *outputs,
	U32 vendor, U32 product )
{
	struct MsgPort *port;
	struct IOStdReq *request;
	struct AudioFormat format;
	struct PreviewAudioChoices *choices;

	U32 unit;
	U32 index;
	S32 output;

	inputs->Labels[0] = "No audio input";
	outputs->Labels[0] = "No audio output";

	for ( index = 0; index < PREVIEW_AUDIO_CHOICES; index++ )
	{
		outputs->Rates[index].Labels[0] = "Default rate";
		outputs->Rates[index].Count = 1;
	}

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		return;
	}

	request = AudioRequest( port );

	if ( request )
	{
		for ( unit = 0; unit < AUDIO_MAX_UNITS; unit++ )
		{
			if ( OpenDevice( AUDIO_DEVICE_NAME, unit, (PTR) request, 0 ) )
			{
				continue;
			}

			if ( QueryAudioFormat( request, & format ) )
			{
				output = unit >= AUDIO_PLAYBACK_UNIT_BASE;
				choices = output ? outputs : inputs;
				index = choices->Count++;
				choices->Units[index] = unit;
				choices->USBIDs[index] = ( format.VendorID << 16 ) | format.DeviceID;
				choices->Labels[index] = choices->Names[index];
				choices->Labels[index + 1] = NULL;

				snprintf( choices->Names[index], sizeof( choices->Names[index] ), "%s %lu (%04lx:%04lx)",
					output ? "Out" : "In", (unsigned long) unit,
					(unsigned long) format.VendorID, (unsigned long) format.DeviceID );

				if ( ! output )
				{
					ReadInputGain( request, inputs, index );
				}
				else
				{
					ReadOutputRates( request, & outputs->Rates[index] );
				}
			}

			CloseDevice( (PTR) request );
		}

		FreeSysObject( ASOT_IOREQUEST, request );
	}

	request = OpenAHI( port );

	if ( request )
	{
		index = outputs->Count++;
		outputs->Labels[index] = "AHI";
		outputs->Units[index] = PREVIEW_AUDIO_AHI;

		CloseDevice( (PTR) request );

		FreeSysObject( ASOT_IOREQUEST, request );
	}

	FreeSysObject( ASOT_PORT, port );

	PreviewAudio_SelectInput( inputs, vendor, product );
}

// --

static void
CloseAudioStream( struct PreviewAudioStream *stream )
{
	if ( stream->Request )
	{
		if ( stream->Pending )
		{
			if ( ! CheckIO( (PTR) stream->Request ) )
			{
				AbortIO( (PTR) stream->Request );
			}

			WaitIO( (PTR) stream->Request );

			stream->Pending = FALSE;
		}

		if ( stream->OriginalRate )
		{
			AudioRateIO( stream->Request, AUDIO_CMD_SET_RATE, stream->OriginalRate );

			stream->OriginalRate = 0;
		}

		CloseDevice( (PTR) stream->Request );

		FreeSysObject( ASOT_IOREQUEST, stream->Request );

		stream->Request = NULL;
	}

	FreeVec( stream->Buffer );

	FreeVec( stream->Spare );

	stream->Buffer = NULL;
	stream->Spare = NULL;
	stream->AHI = FALSE;
}

// --

static S32
OpenAudioStream( struct PreviewAudioStream *stream, struct MsgPort *port, U32 unit )
{
	S32 error;

	stream->Request = AudioRequest( port );

	if ( ! stream->Request )
	{
		return ( FALSE );
	}

	error = OpenDevice( AUDIO_DEVICE_NAME, unit, (PTR) stream->Request, 0 );

	if ( error )
	{
		FreeSysObject( ASOT_IOREQUEST, stream->Request );

		stream->Request = NULL;

		return ( FALSE );
	}

	if ( ! QueryAudioFormat( stream->Request, & stream->Format ) )
	{
		CloseAudioStream( stream );

		return ( FALSE );
	}

	stream->Frames = MAX( 1, (U64) stream->Format.SampleRate * PREVIEW_AUDIO_CHUNK_US / 1000000U );

	return ( TRUE );
}

// --

static void
SendAudio( struct PreviewAudioStream *stream, U32 command, U32 bytes )
{
	stream->Request->io_Command = command;
	stream->Request->io_Data = stream->Buffer;
	stream->Request->io_Length = bytes;
	stream->Request->io_Offset = 0;
	stream->Request->io_Flags = 0;
	stream->Pending = TRUE;

	SendIO( (PTR) stream->Request );
}

// --

static void
CloseMonitor( struct PreviewAudio *audio )
{
	CloseAudioStream( & audio->Output );

	FreeVec( audio->MonitorBuffer );

	audio->MonitorBuffer = NULL;
	audio->MonitorRead = 0;
	audio->MonitorCount = 0;
	audio->ShortWrites = 0;
	audio->MonitorPrimed = FALSE;
}

// --

static void
UpdateRecording( struct PreviewAudio *audio, U8 *buffer, U32 bytes );

// --

static S32
ValidInput( const struct PreviewAudioStream *input, S32 error, U32 bytes )
{
	U32 frameBytes;

	frameBytes = input->Format.Channels * 2;

	return ( ( ( ! error ) || ( error == AUDIO_ERR_SHORT_READ ) ) && ( frameBytes ) &&
		( bytes ) && ( bytes <= input->Frames * frameBytes ) && ( bytes % frameBytes == 0 ) );
}

// --

static void
FinishRecording( struct PreviewAudio *audio )
{
	U8 header[AUDIO_WAV_HEADER_SIZE];
	S32 saved;
	U32 end;

	audio->Record = FALSE;

	if ( ! audio->File )
	{
		return;
	}

	Audio_Wav_Header( header, audio->Written, audio->Input.Format.SampleRate, audio->Input.Format.Channels );

	end = AUDIO_WAV_HEADER_SIZE + audio->Written;

	if ( GetFileSize( audio->File ) != end )
	{
		ChangeFileSize( audio->File, end, OFFSET_BEGINNING );
	}

	saved = ( GetFileSize( audio->File ) == end ) &&
		( ChangeFilePosition( audio->File, 0, OFFSET_BEGINNING ) ) &&
		( Write( audio->File, header, sizeof( header ) ) == sizeof( header ) );

	if ( ! Close( audio->File ) )
	{
		saved = FALSE;
	}

	audio->File = 0;

	if ( ! saved )
	{
		audio->RecordingResult = PREVIEW_RECORD_FAILED;
	}
	else if ( audio->Written )
	{
		audio->RecordingResult = audio->WriteFailed ? PREVIEW_RECORD_PARTIAL : PREVIEW_RECORD_SAVED;
	}
	else if ( audio->WriteFailed )
	{
		audio->RecordingResult = PREVIEW_RECORD_FAILED;
	}
}

// --

void
PreviewAudio_Stop( struct PreviewAudio *audio )
{
	struct PreviewAudioStream *input;

	S32 error;
	U32 bytes;

	input = & audio->Input;

	if ( ( input->Request ) && ( input->Pending ) )
	{
		if ( ! CheckIO( (PTR) input->Request ) )
		{
			AbortIO( (PTR) input->Request );
		}

		error = WaitIO( (PTR) input->Request );
		input->Pending = FALSE;
		bytes = input->Request->io_Actual;

		if ( ValidInput( input, error, bytes ) )
		{
			UpdateRecording( audio, input->Buffer, bytes );
		}
	}

	CloseAudioStream( input );

	if ( audio->ShortReads )
	{
		Printf( "Preview audio lost USB intervals in %lu buffers.\n", audio->ShortReads );

		audio->ShortReads = 0;
	}

	CloseMonitor( audio );

	FinishRecording( audio );

	if ( audio->Port )
	{
		FreeSysObject( ASOT_PORT, audio->Port );

		audio->Port = NULL;
	}
}

// --

static void
SetInputGain( struct IOStdReq *request, const struct PreviewAudioChoices *inputs )
{
	const struct AudioGain *gain;

	U32 index;
	S32 level;
	S32 steps;
	S32 value;

	index = inputs->Selected;
	gain = & inputs->Gains[index];

	if ( ( ! inputs->GainChanged[index] ) || ( ! gain->Step ) )
	{
		return;
	}

	steps = ( gain->Maximum - gain->Minimum ) / gain->Step;
	level = inputs->GainLevels[index];
	value = gain->Minimum + ( ( steps * level + 50 ) / 100 ) * gain->Step;
	request->io_Command = AUDIO_CMD_SET_GAIN;
	request->io_Data = & value;
	request->io_Length = sizeof( value );

	if ( DoIO( (PTR) request ) )
	{
		Printf( "Could not set microphone gain on audio input unit %lu.\n", inputs->Units[index] );
	}
}

// --

void
PreviewAudio_Start( struct PreviewAudio *audio, const struct PreviewAudioChoices *inputs,
	S32 monitor, const struct PreviewAudioChoices *outputs, const char *path )
{
	const struct PreviewAudioRates *rates;
	struct PreviewAudioStream *sink;
	struct AHIRequest *request;

	U32 bytes;
	U32 outputBytes;
	U32 input;
	U32 output;
	U32 rate;
	S32 error;

	if ( ( ! monitor ) && ( ! path ) )
	{
		return;
	}

	audio->Path = path;
	audio->Written = 0;
	bytes = 0;
	input = inputs->Units[inputs->Selected];

	audio->Port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! audio->Port )
	{
		Printf( "Could not allocate the preview audio message port.\n" );

		return;
	}

	if ( OpenAudioStream( & audio->Input, audio->Port, input ) )
	{
		SetInputGain( audio->Input.Request, inputs );

		bytes = audio->Input.Frames * audio->Input.Format.Channels * 2;

		audio->Input.Buffer = AllocVecTags( bytes, TAG_END );

		audio->Input.Spare = AllocVecTags( bytes, TAG_END );
	}

	if ( ( ! audio->Input.Buffer ) || ( ! audio->Input.Spare ) )
	{
		Printf( "Could not start audio input unit %lu.\n", input );

		PreviewAudio_Stop( audio );

		return;
	}

	if ( monitor )
	{
		sink = & audio->Output;
		output = outputs->Units[outputs->Selected];
		rates = & outputs->Rates[outputs->Selected];
		rate = rates->Values[rates->Selected];

		if ( output == PREVIEW_AUDIO_AHI )
		{
			sink->Request = OpenAHI( audio->Port );

			if ( sink->Request )
			{
				request = (PTR) sink->Request;
				sink->AHI = TRUE;
				sink->Format = audio->Input.Format;
				request->ahir_Type = ( sink->Format.Channels == 1 ) ? AHIST_M16S : AHIST_S16S;
				request->ahir_Frequency = sink->Format.SampleRate;
				request->ahir_Volume = 0x10000;
				request->ahir_Position = 0x8000;
				request->ahir_Link = NULL;
			}
		}
		else if ( ( OpenAudioStream( sink, audio->Port, output ) ) &&
			( rate ) && ( rate != sink->Format.SampleRate ) )
		{
			error = AudioRateIO( sink->Request, AUDIO_CMD_SET_RATE, rate );

			if ( error )
			{
				Printf( "Could not set USB audio output to %lu Hz (error %ld).\n", rate, error );

				CloseAudioStream( sink );
			}
			else
			{
				sink->OriginalRate = sink->Format.SampleRate;
				sink->Format.SampleRate = rate;
			}
		}

		if ( sink->Request )
		{
			// Round up the buffer to cover the converted samples.
			sink->Frames = ( (U64) audio->Input.Frames * sink->Format.SampleRate +
				audio->Input.Format.SampleRate - 1 ) / audio->Input.Format.SampleRate;
			outputBytes = sink->Frames * sink->Format.Channels * 2;
			audio->MonitorStride = outputBytes;

			sink->Buffer = AllocVecTags( outputBytes * ( sink->AHI ? 1 : PREVIEW_USB_WRITE_BLOCKS ), TAG_END );

			audio->MonitorBuffer = AllocVecTags( outputBytes * PREVIEW_MONITOR_BUFFERS, TAG_END );
		}

		if ( ( ! sink->Buffer ) || ( ! audio->MonitorBuffer ) )
		{
			Printf( "Could not start the selected audio output.\n" );

			CloseMonitor( audio );
		}
	}

	audio->Record = path != NULL;

	if ( ( audio->Output.Request ) || ( audio->Record ) )
	{
		SendAudio( & audio->Input, CMD_READ, bytes );
	}
	else
	{
		PreviewAudio_Stop( audio );
	}
}

// --

static void
UpdateRecording( struct PreviewAudio *audio, U8 *buffer, U32 bytes )
{
	U8 header[AUDIO_WAV_HEADER_SIZE];
	S32 written;
	U32 frameBytes;

	if ( ! audio->Record )
	{
		return;
	}

	if ( ! audio->File )
	{
		audio->File = Open( audio->Path, MODE_NEWFILE );

		if ( audio->File )
		{
			Audio_Wav_Header( header, 0, audio->Input.Format.SampleRate, audio->Input.Format.Channels );

			if ( Write( audio->File, header, sizeof( header ) ) != sizeof( header ) )
			{
				Close( audio->File );

				audio->File = 0;
			}
		}

		if ( ! audio->File )
		{
			audio->Record = FALSE;
			audio->RecordingResult = PREVIEW_RECORD_FAILED;

			Printf( "Could not start recording to %s.\n", audio->Path );

			return;
		}
	}

	written = 0;
	frameBytes = audio->Input.Format.Channels * 2;

	if ( bytes <= ~0U - AUDIO_WAV_HEADER_SIZE - audio->Written )
	{
		written = Write( audio->File, buffer, bytes );

		if ( written > 0 )
		{
			audio->Written += written - written % frameBytes;
		}
	}

	if ( written != (S32) bytes )
	{
		audio->WriteFailed = TRUE;

		Printf( "Audio recording stopped after a write failure or WAV size limit.\n" );

		FinishRecording( audio );
	}
}

// --

static void
ReportAudioBandwidth( S32 error )
{
	if ( error == USB2Err_Host_NoBandWidth )
	{
		Printf( "This controller could not schedule the selected audio stream alongside the other active streams.\n" );
	}
}

// --

static void
ScaleMonitorSamples( U8 *data, U32 bytes, U32 volume, S32 ahi )
{
	U32 index;
	U32 high;
	U32 low;
	S32 sample;
	U16 value;

	if ( volume >= 100 )
	{
		return;
	}

	// AHI uses native big-endian words. USB playback uses little-endian PCM.
	high = ahi ? 0 : 1;
	low = 1 - high;

	for ( index = 0; index + 1 < bytes; index += 2 )
	{
		sample = ( data[index + high] << 8 ) | data[index + low];

		if ( sample >= 32768 )
		{
			sample -= 65536;
		}

		value = (U16) ( sample * (S32) volume / 100 );
		data[index + high] = value >> 8;
		data[index + low] = value;
	}
}

// --

static void
PlayQueuedAudio( struct PreviewAudio *audio )
{
	struct PreviewAudioStream *output;

	U32 slot;
	U32 bytes;
	U32 blocks;
	U32 total;

	output = & audio->Output;

	if ( ( output->Pending ) || ( ! audio->MonitorCount ) ||
		( ( ! audio->MonitorPrimed ) && ( audio->MonitorCount < 2 ) ) )
	{
		return;
	}

	audio->MonitorPrimed = TRUE;
	total = 0;
	blocks = output->AHI ? 1 : MIN( audio->MonitorCount, PREVIEW_USB_WRITE_BLOCKS );

	while ( blocks-- )
	{
		slot = audio->MonitorRead;
		bytes = audio->MonitorLengths[slot];
		audio->MonitorRead = ( slot + 1 ) % PREVIEW_MONITOR_BUFFERS;
		audio->MonitorCount--;

		CopyMem( audio->MonitorBuffer + slot * audio->MonitorStride, output->Buffer + total, bytes );

		total += bytes;
	}

	ScaleMonitorSamples( output->Buffer, total, audio->Volume, output->AHI );

	SendAudio( output, CMD_WRITE, total );
}

// --

static void
QueueMonitorAudio( struct PreviewAudio *audio, U8 *input, U32 bytes )
{
	struct PreviewAudioStream *output;
	struct AudioWav samples;

	U8 *buffer;
	U32 slot;
	U32 index;
	U32 frames;

	output = & audio->Output;

	if ( ! output->Request )
	{
		return;
	}

	if ( audio->MonitorCount == PREVIEW_MONITOR_BUFFERS )
	{
		// Keep monitoring close to live if the output falls behind.
		audio->MonitorRead = ( audio->MonitorRead + 1 ) % PREVIEW_MONITOR_BUFFERS;
		audio->MonitorCount--;
	}

	slot = ( audio->MonitorRead + audio->MonitorCount ) % PREVIEW_MONITOR_BUFFERS;
	buffer = audio->MonitorBuffer + slot * audio->MonitorStride;

	if ( output->AHI )
	{
		// USB input and WAV use little-endian PCM. AHI expects native signed words.
		for ( index = 0; index < bytes; index += 2 )
		{
			buffer[index] = input[index + 1];
			buffer[index + 1] = input[index];
		}
	}
	else
	{
		samples.Samples = input;
		samples.Rate = audio->Input.Format.SampleRate;
		samples.Channels = audio->Input.Format.Channels;
		samples.Frames = bytes / ( samples.Channels * 2 );
		frames = Audio_Wav_ConvertBlock( & audio->MonitorResampler, & samples, buffer,
			output->Format.SampleRate, output->Format.Channels );

		if ( ! frames )
		{
			return;
		}

		bytes = frames * output->Format.Channels * 2;
	}

	audio->MonitorLengths[slot] = bytes;
	audio->MonitorCount++;
}

// --

static void
FinishMonitorWrite( struct PreviewAudio *audio )
{
	struct PreviewAudioStream *output;

	S32 error;
	S32 partial;
	U32 actual;
	U32 wanted;

	output = & audio->Output;

	error = WaitIO( (PTR) output->Request );

	output->Pending = FALSE;
	actual = output->Request->io_Actual;
	wanted = output->Request->io_Length;
	partial = ( ! output->AHI ) && ( ( error == AUDIO_ERR_SHORT_WRITE ) ||
		( error == IOERR_BADLENGTH ) || ( ( ! error ) && ( actual != wanted ) ) );

	if ( ( ! error ) && ( ! partial ) )
	{
		audio->ShortWrites = 0;

		return;
	}

	if ( ( partial ) && ( ++audio->ShortWrites < 3 ) )
	{
		if ( audio->ShortWrites == 1 )
		{
			Printf( "USB audio short write (error %ld, %lu of %lu bytes). Continuing monitoring.\n",
				error, actual, wanted );
		}

		return;
	}

	Printf( "Audio monitoring stopped (output error %ld, %lu of %lu bytes).\n", error, actual, wanted );

	ReportAudioBandwidth( error );

	CloseMonitor( audio );
}

// --

void
PreviewAudio_Update( struct PreviewAudio *audio )
{
	struct PreviewAudioStream *input;
	struct PreviewAudioStream *output;

	U8 *ready;
	U32 bytes;
	U32 wanted;
	S32 error;

	input = & audio->Input;
	output = & audio->Output;

	if ( ( output->Pending ) && ( CheckIO( (PTR) output->Request ) ) )
	{
		FinishMonitorWrite( audio );
	}

	if ( ( ! audio->Record ) && ( ! output->Request ) )
	{
		PreviewAudio_Stop( audio );

		return;
	}

	PlayQueuedAudio( audio );

	if ( ( ! input->Pending ) || ( ! CheckIO( (PTR) input->Request ) ) )
	{
		return;
	}

	error = WaitIO( (PTR) input->Request );

	input->Pending = FALSE;
	bytes = input->Request->io_Actual;
	wanted = input->Frames * input->Format.Channels * 2;

	if ( ! ValidInput( input, error, bytes ) )
	{
		Printf( "Preview audio input stopped (error %ld, %lu bytes).\n", error, bytes );

		ReportAudioBandwidth( error );

		PreviewAudio_Stop( audio );

		return;
	}

	if ( error == AUDIO_ERR_SHORT_READ )
	{
		audio->ShortReads++;
	}

	// Keep the microphone running while converting and saving the previous chunk.
	ready = input->Buffer;
	input->Buffer = input->Spare;
	input->Spare = ready;

	SendAudio( input, CMD_READ, wanted );

	UpdateRecording( audio, ready, bytes );

	QueueMonitorAudio( audio, ready, bytes );

	PlayQueuedAudio( audio );

	if ( ( ! audio->Record ) && ( ! output->Request ) )
	{
		PreviewAudio_Stop( audio );
	}
}

// --
