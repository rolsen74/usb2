
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <string.h>
#include <exec/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/ahi.h>
#include <devices/usb2.h>
#include "../inc/VideoDevice.h"
#include "Preview.h"
#include "PreviewAudio.h"

// --

#define PREVIEW_AUDIO_CHUNK_US 32000U
#define PREVIEW_AUDIO_AHI ~0U
#define PREVIEW_MONITOR_RATE_ADJUSTMENT 10
#define PREVIEW_MONITOR_RECOVERY_WINDOW 64U
#define PREVIEW_MONITOR_RECOVERY_COOLDOWN 32U

// --

static struct IOStdReq *
AudioRequest( struct MsgPort *port )
{
	return ( (PTR) AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( struct IOStdReq ),
		ASOIOR_ReplyPort, port, TAG_END ) );
}

// --

static S32
QueryAudioFormat( struct IOStdReq *request, struct AudioFormat *format, S32 capture )
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
		( ( format->Channels == 1 ) || ( format->Channels == 2 ) ||
			( ( capture ) && ( format->Channels == 4 ) ) ) &&
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

			output = unit >= AUDIO_PLAYBACK_UNIT_BASE;

			if ( QueryAudioFormat( request, & format, ! output ) )
			{
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

	if ( ! QueryAudioFormat( stream->Request, & stream->Format, unit < AUDIO_PLAYBACK_UNIT_BASE ) )
	{
		CloseAudioStream( stream );

		return ( FALSE );
	}

	stream->Frames = MAX( 1, (U64) stream->Format.SampleRate * PREVIEW_AUDIO_CHUNK_US / 1000000U );

	return ( TRUE );
}

// --

static void
SendAudioRequest( struct IOStdReq *request, U8 *buffer, U32 command, U32 bytes )
{
	request->io_Command = command;
	request->io_Data = buffer;
	request->io_Length = bytes;
	request->io_Offset = 0;
	request->io_Flags = 0;

	SendIO( (PTR) request );
}

// --

static void
OpenMonitorRequests( struct PreviewAudio *audio )
{
	struct PreviewAudioStream *output;
	struct IOStdReq *request;

	U32 maximum;

	output = &audio->Output;
	maximum = output->AHI ? 1 : Audio_QueuedWriteDepth( &output->Request->io_Device->dd_Library );
	audio->Writes[0] = output->Request;
	audio->WriteDepth = 1;

	while ( audio->WriteDepth < maximum )
	{
		request = AudioRequest( audio->Port );

		if ( ! request )
		{
			break;
		}

		request->io_Device = output->Request->io_Device;
		request->io_Unit = output->Request->io_Unit;
		audio->Writes[audio->WriteDepth++] = request;
	}
}

// --

static void
StartAudioInput( struct PreviewAudio *audio, U32 bytes )
{
	struct PreviewAudioStream *stream;

	stream = &audio->Input;
	stream->Pending = TRUE;

	SendAudioRequest( stream->Request, stream->Buffer, audio->TimedRead ? AUDIO_CMD_READ_TIMED : CMD_READ, bytes );
}

// --

static void
ResetMonitorQueue( struct PreviewAudio *audio )
{
	audio->MonitorResampler = (struct AudioWavResampler){ 0 };
	audio->MonitorRead = 0;
	audio->MonitorCount = 0;
	audio->MonitorOffset = 0;
	audio->MonitorBytes = 0;
	audio->MonitorPrimed = FALSE;
}

// --

static void
CloseMonitor( struct PreviewAudio *audio )
{
	struct IOStdReq *request;

	U32 index;
	U32 slot;

	for ( index = audio->WriteCount; index; index-- )
	{
		slot = ( audio->WriteHead + index - 1 ) % audio->WriteDepth;
		request = audio->Writes[slot];

		if ( ! CheckIO( (PTR) request ) )
		{
			AbortIO( (PTR) request );
		}
	}

	for ( index = 0; index < audio->WriteCount; index++ )
	{
		slot = ( audio->WriteHead + index ) % audio->WriteDepth;

		WaitIO( (PTR) audio->Writes[slot] );
	}

	for ( index = 1; index < audio->WriteDepth; index++ )
	{
		FreeSysObject( ASOT_IOREQUEST, audio->Writes[index] );

		audio->Writes[index] = NULL;
	}

	audio->Writes[0] = NULL;
	audio->WriteDepth = 0;
	audio->WriteHead = 0;
	audio->WriteCount = 0;

	CloseAudioStream( & audio->Output );

	FreeVec( audio->MonitorBuffer );

	FreeVec( audio->Silence );

	audio->MonitorBuffer = NULL;
	audio->Silence = NULL;
	audio->MonitorPhase = 0;
	audio->SilencePhase = 0;
	audio->MonitorEmptySince = 0;
	audio->ShortWrites = 0;
	audio->MonitorRecovering = FALSE;

	ResetMonitorQueue( audio );
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
		( ( bytes ) || ( ( error == AUDIO_ERR_SHORT_READ ) && ( input->Request->io_Offset ) &&
			( input->Request->io_Command == AUDIO_CMD_READ_TIMED ) ) ) &&
		( bytes <= input->Frames * frameBytes ) && ( bytes % frameBytes == 0 ) );
}

// --

static void
FinishRecording( struct PreviewAudio *audio )
{
	U8 header[AUDIO_WAV_MAX_HEADER_SIZE];
	S32 saved;
	U32 end;
	U32 headerBytes;

	audio->Record = FALSE;

	if ( ! audio->File )
	{
		return;
	}

	headerBytes = Audio_Wav_Header( header, audio->Written, audio->Input.Format.SampleRate,
		audio->Input.Format.Channels );

	end = headerBytes + audio->Written;

	if ( GetFileSize( audio->File ) != end )
	{
		ChangeFileSize( audio->File, end, OFFSET_BEGINNING );
	}

	saved = ( GetFileSize( audio->File ) == end ) &&
		( ChangeFilePosition( audio->File, 0, OFFSET_BEGINNING ) ) &&
		( Write( audio->File, header, headerBytes ) == (S32) headerBytes );

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

	U64 elapsed;
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
			if ( error == AUDIO_ERR_SHORT_READ )
			{
				audio->ShortReads++;
			}

			audio->CapturedFrames += bytes / ( input->Format.Channels * 2 );

			if ( bytes )
			{
				UpdateRecording( audio, input->Buffer, bytes );
			}
		}
	}

	CloseAudioStream( input );

	if ( ( audio->Trace ) && ( audio->Port ) )
	{
		elapsed = audio->Started ? VideoTest_UptimeUS( audio->Clock ) - audio->Started : 0;

		DebugPrintF( "USB2VideoTest: audio input %lu Hz, output %lu Hz, %lu input buffers with USB packet loss, "
			"%lu bytes of silence added, "
			"%lu short output writes, %lu monitor underruns, %lu dropped monitor blocks\n",
			audio->Input.Format.SampleRate, audio->Output.Format.SampleRate, audio->ShortReads, audio->PaddedBytes,
			audio->ShortWriteTotal, audio->MonitorUnderruns, audio->MonitorDrops );

		DebugPrintF( "USB2VideoTest: audio in %lu ms captured %lu frames, completed %lu output frames, "
			"longest observed refill %lu us\n",
			(U32) ( elapsed / 1000 ), (U32) audio->CapturedFrames, (U32) audio->PlayedFrames, (U32) audio->MonitorRefillUS );

		DebugPrintF( "USB2VideoTest: audio USB output gaps in %lu writes, %lu monitor recoveries\n",
			audio->MonitorGapWrites, audio->MonitorRecoveries );
	}

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
	U64 minimumStep;
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

		audio->TimedRead = monitor && LIB_IS_AT_LEAST( &audio->Input.Request->io_Device->dd_Library,
			AUDIO_DEVICE_VERSION, AUDIO_TIMED_READ_REVISION );
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
				sink->Format.Channels = MIN( sink->Format.Channels, 2 );
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
			OpenMonitorRequests( audio );

			audio->MonitorCanRecover = ( ! sink->AHI ) && ( audio->WriteDepth > 1 ) &&
				LIB_IS_AT_LEAST( &sink->Request->io_Device->dd_Library, AUDIO_DEVICE_VERSION, AUDIO_OUTPUT_RECOVERY_REVISION );

			sink->Frames = audio->Input.Frames;

			if ( ! sink->AHI )
			{
				minimumStep = (U64) audio->Input.Format.SampleRate *
					( AUDIO_WAV_RATE_SCALE - PREVIEW_MONITOR_RATE_ADJUSTMENT );
				sink->Frames = ( (U64) audio->Input.Frames * sink->Format.SampleRate * AUDIO_WAV_RATE_SCALE +
					minimumStep - 1 ) / minimumStep;
				sink->Frames = MAX( sink->Frames,
					( (U64) sink->Format.SampleRate * PREVIEW_AUDIO_CHUNK_US + 999999 ) / 1000000 );
			}

			outputBytes = sink->Frames * sink->Format.Channels * 2;
			audio->MonitorStride = outputBytes;

			sink->Buffer = AllocVecTags( outputBytes * audio->WriteDepth, TAG_END );

			audio->MonitorBuffer = AllocVecTags( outputBytes * PREVIEW_MONITOR_BUFFERS, TAG_END );

			if ( audio->TimedRead )
			{
				audio->Silence = AllocVecTags( bytes, AVT_ClearWithValue, 0, TAG_END );
			}
		}

		if ( ( ! sink->Buffer ) || ( ! audio->MonitorBuffer ) || ( ( audio->TimedRead ) && ( ! audio->Silence ) ) )
		{
			Printf( "Could not start the selected audio output.\n" );

			CloseMonitor( audio );
		}
	}

	audio->Record = path != NULL;
	audio->TimedRead = audio->TimedRead && audio->Output.Request != NULL;
	audio->MonitorVolume = audio->Volume;

	if ( ( audio->Output.Request ) || ( audio->Record ) )
	{
		if ( ( audio->Trace ) && ( audio->Clock ) )
		{
			audio->Started = VideoTest_UptimeUS( audio->Clock );
		}

		StartAudioInput( audio, bytes );
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
	U8 header[AUDIO_WAV_MAX_HEADER_SIZE];
	S32 written;
	U32 frameBytes;
	U32 headerBytes;

	if ( ! audio->Record )
	{
		return;
	}

	headerBytes = Audio_Wav_HeaderSize( audio->Input.Format.Channels );

	if ( ! audio->File )
	{
		audio->File = Open( audio->Path, MODE_NEWFILE );

		if ( audio->File )
		{
			headerBytes = Audio_Wav_Header( header, 0, audio->Input.Format.SampleRate,
				audio->Input.Format.Channels );

			if ( Write( audio->File, header, headerBytes ) != (S32) headerBytes )
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

	if ( bytes <= ~0U - headerBytes - audio->Written )
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
ScaleMonitorSamples( struct PreviewAudio *audio, U8 *data, U32 bytes )
{
	U32 index;
	U32 channel;
	U32 frameBytes;
	U32 high;
	U32 low;
	U32 ramp;
	U32 remaining;
	S32 gain;
	S32 target;
	S32 change;
	S32 sample;
	U16 value;

	target = audio->Volume * 256;
	change = target - (S32) audio->MonitorVolume * 256;
	gain = target;
	audio->MonitorVolume = audio->Volume;

	if ( ( ! change ) && ( audio->Volume == 100 ) )
	{
		return;
	}

	frameBytes = audio->Output.Format.Channels * 2;
	ramp = change ? MIN( bytes / frameBytes, MAX( 1U, audio->Output.Format.SampleRate / 200 ) ) : 0;
	remaining = ramp;

	// AHI uses native big-endian words. USB playback uses little-endian PCM.
	high = audio->Output.AHI ? 0 : 1;
	low = 1 - high;

	for ( index = 0; index < bytes; index += frameBytes )
	{
		if ( remaining )
		{
			remaining--;
			gain = target - (S64) change * remaining / ramp;
		}

		for ( channel = index; channel < index + frameBytes; channel += 2 )
		{
			sample = ( data[channel + high] << 8 ) | data[channel + low];

			if ( sample >= 32768 )
			{
				sample -= 65536;
			}

			value = (U16) ( sample * gain / 25600 );
			data[channel + high] = value >> 8;
			data[channel + low] = value;
		}
	}
}

// --

static void
MonitorWriteSize( const struct PreviewAudio *audio, U32 blocks, U32 *bytes, U32 *phase )
{
	U64 samples;

	if ( audio->Output.AHI )
	{
		*bytes = audio->MonitorCount ? audio->MonitorLengths[audio->MonitorRead] - audio->MonitorOffset : 0;
		*phase = 0;

		return;
	}

	// Match the USB packet cadence across resampled input blocks.
	samples = (U64) audio->Output.Format.SampleRate * PREVIEW_AUDIO_CHUNK_US * blocks + audio->MonitorPhase;
	*bytes = ( samples / 1000000U ) * audio->Output.Format.Channels * 2;
	*phase = samples % 1000000U;
}

// --

static void
PlayQueuedAudio( struct PreviewAudio *audio )
{
	U8 *buffer;
	U32 write;
	U32 read;
	U32 bytes;
	U32 copied;
	U32 part;
	U32 command;
	U32 priming;
	U32 phase;
	U64 now;

	priming = MAX( 2, audio->WriteDepth );

	if ( ( audio->MonitorRecovering ) || ( audio->WriteCount == audio->WriteDepth ) || ( ! audio->MonitorCount ) ||
		( ( ! audio->MonitorPrimed ) && ( audio->MonitorCount < priming ) ) )
	{
		return;
	}

	if ( ( ! audio->MonitorPrimed ) && ( ! audio->Output.AHI ) && ( audio->WriteDepth > 1 ) )
	{
		// The resampler can leave the first few input blocks a few samples short.
		MonitorWriteSize( audio, audio->WriteDepth, &bytes, &phase );

		if ( audio->MonitorBytes < bytes )
		{
			return;
		}
	}

	command = ( audio->WriteDepth > 1 ) ? AUDIO_CMD_QUEUE_WRITE : CMD_WRITE;

	while ( ( audio->WriteCount < audio->WriteDepth ) && ( audio->MonitorCount ) )
	{
		MonitorWriteSize( audio, 1, &bytes, &phase );

		if ( ( ! bytes ) || ( bytes > audio->MonitorBytes ) )
		{
			break;
		}

		write = ( audio->WriteHead + audio->WriteCount ) % audio->WriteDepth;
		buffer = audio->Output.Buffer + write * audio->MonitorStride;
		copied = 0;

		while ( copied < bytes )
		{
			read = audio->MonitorRead;
			part = MIN( bytes - copied, audio->MonitorLengths[read] - audio->MonitorOffset );

			CopyMem( audio->MonitorBuffer + read * audio->MonitorStride + audio->MonitorOffset, buffer + copied, part );

			copied += part;
			audio->MonitorOffset += part;

			if ( audio->MonitorOffset == audio->MonitorLengths[read] )
			{
				audio->MonitorOffset = 0;
				audio->MonitorRead = ( read + 1 ) % PREVIEW_MONITOR_BUFFERS;
				audio->MonitorCount--;
			}
		}

		audio->MonitorBytes -= bytes;
		audio->MonitorPhase = phase;
		audio->MonitorPrimed = TRUE;

		ScaleMonitorSamples( audio, buffer, bytes );

		audio->WriteCount++;

		SendAudioRequest( audio->Writes[write], buffer, command, bytes );

		if ( audio->MonitorEmptySince )
		{
			now = VideoTest_UptimeUS( audio->Clock );
			audio->MonitorRefillUS = MAX( audio->MonitorRefillUS, now - audio->MonitorEmptySince );
			audio->MonitorEmptySince = 0;
		}
	}
}

// --

static void
NoteMonitorDisruption( struct PreviewAudio *audio )
{
	if ( ( ! audio->MonitorCanRecover ) || ( audio->MonitorRecovering ) || ( audio->MonitorRecoveryCooldown ) )
	{
		return;
	}

	if ( ! audio->MonitorRecoveryWindow )
	{
		audio->MonitorRecoveryWindow = PREVIEW_MONITOR_RECOVERY_WINDOW;

		return;
	}

	// Drain submitted writes while capture continues, then prime with recent audio.
	audio->MonitorRecovering = TRUE;
	audio->MonitorRecoveryWindow = 0;

	ResetMonitorQueue( audio );
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
	S32 adjustment;
	S32 queued;

	output = & audio->Output;

	if ( ! output->Request )
	{
		return;
	}

	if ( audio->MonitorCount == PREVIEW_MONITOR_BUFFERS )
	{
		// Keep monitoring close to live if the output falls behind.
		audio->MonitorBytes -= audio->MonitorLengths[audio->MonitorRead] - audio->MonitorOffset;
		audio->MonitorOffset = 0;
		audio->MonitorRead = ( audio->MonitorRead + 1 ) % PREVIEW_MONITOR_BUFFERS;
		audio->MonitorCount--;
		audio->MonitorDrops++;

		NoteMonitorDisruption( audio );
	}

	slot = ( audio->MonitorRead + audio->MonitorCount ) % PREVIEW_MONITOR_BUFFERS;
	buffer = audio->MonitorBuffer + slot * audio->MonitorStride;

	samples.Samples = input;
	samples.Rate = audio->Input.Format.SampleRate;
	samples.Channels = audio->Input.Format.Channels;
	samples.Frames = bytes / ( samples.Channels * 2 );
	adjustment = 0;

	if ( ( ! output->AHI ) && ( audio->MonitorPrimed ) )
	{
		// Correct small clock differences around one block of queued PCM.
		queued = audio->MonitorBytes;
		adjustment = ( queued - (S32) audio->MonitorStride ) *
			PREVIEW_MONITOR_RATE_ADJUSTMENT / (S32) audio->MonitorStride;
		adjustment = MIN( PREVIEW_MONITOR_RATE_ADJUSTMENT, MAX( -PREVIEW_MONITOR_RATE_ADJUSTMENT, adjustment ) );
	}

	if ( output->AHI )
	{
		frames = samples.Frames;

		Audio_Wav_Convert( & samples, buffer, frames, output->Format.SampleRate, output->Format.Channels );
	}
	else
	{
		frames = Audio_Wav_ConvertBlock( & audio->MonitorResampler, & samples, buffer,
			output->Format.SampleRate, output->Format.Channels, adjustment );
	}

	if ( ! frames )
	{
		return;
	}

	bytes = frames * output->Format.Channels * 2;

	if ( output->AHI )
	{
		U8 sample;

		// The converter produces little-endian PCM. AHI expects native words.
		for ( index = 0; index < bytes; index += 2 )
		{
			sample = buffer[index];
			buffer[index] = buffer[index + 1];
			buffer[index + 1] = sample;
		}
	}

	audio->MonitorLengths[slot] = bytes;
	audio->MonitorBytes += bytes;
	audio->MonitorCount++;
}

// --

static void
FinishMonitorWrite( struct PreviewAudio *audio )
{
	struct IOStdReq *request;

	S32 error;
	S32 partial;
	U32 actual;
	U32 wanted;
	U32 nextBytes;
	U32 phase;

	request = audio->Writes[audio->WriteHead];

	error = WaitIO( (PTR) request );

	audio->WriteHead = ( audio->WriteHead + 1 ) % audio->WriteDepth;
	audio->WriteCount--;
	actual = request->io_Actual;
	wanted = request->io_Length;
	partial = ( ! audio->Output.AHI ) && ( ( error == AUDIO_ERR_SHORT_WRITE ) ||
		( error == IOERR_BADLENGTH ) || ( ( ! error ) && ( actual != wanted ) ) );
	audio->PlayedFrames += MIN( actual, wanted ) / ( audio->Output.Format.Channels * 2 );

	if ( audio->MonitorRecoveryWindow )
	{
		audio->MonitorRecoveryWindow--;
	}

	if ( audio->MonitorRecoveryCooldown )
	{
		audio->MonitorRecoveryCooldown--;
	}

	if ( ( audio->MonitorCanRecover ) && ( request->io_Offset ) )
	{
		audio->MonitorGapWrites++;

		NoteMonitorDisruption( audio );
	}

	if ( ( ! error ) && ( ! partial ) )
	{
		audio->ShortWrites = 0;

		MonitorWriteSize( audio, 1, &nextBytes, &phase );

		if ( ( audio->MonitorPrimed ) && ( ! audio->WriteCount ) )
		{
			audio->MonitorUnderruns++;

			if ( audio->Clock )
			{
				audio->MonitorEmptySince = VideoTest_UptimeUS( audio->Clock );
			}

			if ( ( ( ! nextBytes ) || ( audio->MonitorBytes < nextBytes ) ) &&
				( ( ! audio->Input.Pending ) || ( ! CheckIO( (PTR) audio->Input.Request ) ) ) )
			{
				audio->MonitorPrimed = FALSE;
			}
		}

		return;
	}

	if ( partial )
	{
		audio->ShortWriteTotal++;
		audio->ShortWrites++;
	}

	if ( ( partial ) && ( audio->ShortWrites < 3 ) )
	{
		if ( ! audio->WriteCount )
		{
			audio->MonitorPrimed = FALSE;
		}

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
	U32 gap;
	U32 frameBytes;
	U32 silenceFrames;
	U32 frames;
	U64 samples;
	S32 error;

	input = & audio->Input;
	output = & audio->Output;

	while ( ( audio->WriteCount ) && ( CheckIO( (PTR) audio->Writes[audio->WriteHead] ) ) )
	{
		FinishMonitorWrite( audio );
	}

	if ( ( ! audio->Record ) && ( ! output->Request ) )
	{
		PreviewAudio_Stop( audio );

		return;
	}

	if ( ( audio->MonitorRecovering ) && ( ! audio->WriteCount ) )
	{
		struct IOStdReq *request;

		request = output->Request;
		request->io_Command = CMD_RESET;
		request->io_Flags = 0;
		request->io_Data = NULL;
		request->io_Length = 0;
		request->io_Offset = 0;

		error = DoIO( (PTR) request );

		if ( error )
		{
			Printf( "Could not restart USB audio monitoring (error %ld).\n", error );

			CloseMonitor( audio );
		}
		else
		{
			audio->MonitorPhase = 0;
			audio->MonitorVolume = 0;
			audio->MonitorRecovering = FALSE;
			audio->MonitorRecoveries++;
			audio->MonitorRecoveryCooldown = PREVIEW_MONITOR_RECOVERY_COOLDOWN;
		}
	}

	PlayQueuedAudio( audio );

	if ( ( ! input->Pending ) || ( ! CheckIO( (PTR) input->Request ) ) )
	{
		return;
	}

	error = WaitIO( (PTR) input->Request );

	input->Pending = FALSE;
	bytes = input->Request->io_Actual;
	frameBytes = input->Format.Channels * 2;
	wanted = input->Frames * frameBytes;
	gap = ( audio->TimedRead && ( error == AUDIO_ERR_SHORT_READ ) ) ? input->Request->io_Offset : 0;

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

	StartAudioInput( audio, wanted );

	audio->CapturedFrames += bytes / frameBytes;

	if ( bytes )
	{
		UpdateRecording( audio, ready, bytes );
	}

	if ( ( output->Request ) && ( ! gap ) && ( bytes < wanted ) )
	{
		memset( ready + bytes, 0, wanted - bytes );

		audio->PaddedBytes += wanted - bytes;
		bytes = wanted;
	}

	if ( bytes )
	{
		QueueMonitorAudio( audio, ready, bytes );
	}

	if ( ( output->Request ) && ( gap ) )
	{
		samples = (U64) gap * input->Format.SampleRate + audio->SilencePhase;
		silenceFrames = samples / 1000000U;
		audio->SilencePhase = samples % 1000000U;

		while ( silenceFrames )
		{
			frames = MIN( silenceFrames, input->Frames );

			PlayQueuedAudio( audio );

			QueueMonitorAudio( audio, audio->Silence, frames * frameBytes );

			audio->PaddedBytes += frames * frameBytes;
			silenceFrames -= frames;
		}
	}

	PlayQueuedAudio( audio );

	if ( ( ! audio->Record ) && ( ! output->Request ) )
	{
		PreviewAudio_Stop( audio );
	}
}

// --
