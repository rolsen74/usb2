#ifndef USB2_VIDEO_TEST_PREVIEW_AUDIO_H
#define USB2_VIDEO_TEST_PREVIEW_AUDIO_H

#include "../../Audio/inc/AudioDevice.h"
#include "PreviewRecording.h"
#include "../../Audio/Test/Wav.h"

#define PREVIEW_AUDIO_CHOICES ( AUDIO_MAX_UNITS - AUDIO_PLAYBACK_UNIT_BASE + 1 )
#define PREVIEW_MONITOR_BUFFERS 4

struct PreviewAudioRates
{
	U32 Values[3];
	STR Labels[4];
	U32 Count;
	U32 Selected;
};

struct PreviewAudioChoices
{
	U32 Units[PREVIEW_AUDIO_CHOICES];
	U32 USBIDs[PREVIEW_AUDIO_CHOICES];
	struct AudioGain Gains[PREVIEW_AUDIO_CHOICES];
	struct PreviewAudioRates Rates[PREVIEW_AUDIO_CHOICES];
	U32 GainLevels[PREVIEW_AUDIO_CHOICES];
	U8 GainChanged[PREVIEW_AUDIO_CHOICES];
	STR Labels[PREVIEW_AUDIO_CHOICES + 1];
	char Names[PREVIEW_AUDIO_CHOICES][24];
	U32 Count;
	U32 Selected;
};

struct PreviewAudioStream
{
	struct IOStdReq *Request;
	struct AudioFormat Format;

	U8 *Buffer;
	U8 *Spare;
	U32 Frames;
	U32 OriginalRate;
	S32 Pending;
	S32 AHI;
};

struct PreviewAudio
{
	struct TimerIFace *Clock;
	struct MsgPort *Port;
	struct PreviewAudioStream Input;
	struct PreviewAudioStream Output;
	struct IOStdReq *Writes[AUDIO_MAX_QUEUED_WRITES];
	struct AudioWavResampler MonitorResampler;

	BPTR File;
	const char *Path;
	U8 *MonitorBuffer;
	U8 *Silence;
	U64 Started;
	U64 MonitorEmptySince;
	U64 MonitorRefillUS;
	U64 CapturedFrames;
	U64 PlayedFrames;
	U32 MonitorStride;
	U32 MonitorLengths[PREVIEW_MONITOR_BUFFERS];
	U32 MonitorRead;
	U32 MonitorCount;
	U32 MonitorOffset;
	U32 MonitorBytes;
	U32 MonitorPhase;
	U32 SilencePhase;
	U32 WriteDepth;
	U32 WriteHead;
	U32 WriteCount;
	U32 ShortReads;
	U32 PaddedBytes;
	U32 ShortWrites;
	U32 ShortWriteTotal;
	U32 MonitorUnderruns;
	U32 MonitorDrops;
	U32 MonitorGapWrites;
	U32 MonitorRecoveries;
	U32 MonitorRecoveryWindow;
	U32 MonitorRecoveryCooldown;
	U32 Written;
	U32 Volume;
	U32 MonitorVolume;
	S32 Record;
	S32 WriteFailed;
	enum PreviewRecordingResult RecordingResult;
	S32 MonitorPrimed;
	S32 MonitorCanRecover;
	S32 MonitorRecovering;
	S32 TimedRead;
	S32 Trace;
};

void PreviewAudio_Find( struct PreviewAudioChoices *inputs, struct PreviewAudioChoices *outputs,
	U32 vendor, U32 product );
void PreviewAudio_SelectInput( struct PreviewAudioChoices *inputs, U32 vendor, U32 product );
void PreviewAudio_Start( struct PreviewAudio *audio, const struct PreviewAudioChoices *inputs,
	S32 monitor, const struct PreviewAudioChoices *outputs, const char *path );
void PreviewAudio_Update( struct PreviewAudio *audio );
void PreviewAudio_Stop( struct PreviewAudio *audio );

#endif
