#ifndef USB2_VIDEO_TEST_PREVIEW_AUDIO_H
#define USB2_VIDEO_TEST_PREVIEW_AUDIO_H

#include "../../Audio/inc/AudioDevice.h"

#define PREVIEW_AUDIO_CHOICES ( AUDIO_MAX_UNITS - AUDIO_PLAYBACK_UNIT_BASE + 1 )
#define PREVIEW_MONITOR_BUFFERS 3

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
	struct MsgPort *Port;
	struct PreviewAudioStream Input;
	struct PreviewAudioStream Output;

	BPTR File;
	const char *Path;
	U8 *MonitorBuffer;
	U32 MonitorStride;
	U32 MonitorLengths[PREVIEW_MONITOR_BUFFERS];
	U32 MonitorRead;
	U32 MonitorCount;
	U32 ShortReads;
	U32 ShortWrites;
	U32 Written;
	U32 Volume;
	S32 Record;
	S32 MonitorPrimed;
};

void PreviewAudio_Find( struct PreviewAudioChoices *inputs, struct PreviewAudioChoices *outputs,
	U32 vendor, U32 product );
void PreviewAudio_SelectInput( struct PreviewAudioChoices *inputs, U32 vendor, U32 product );
void PreviewAudio_Start( struct PreviewAudio *audio, const struct PreviewAudioChoices *inputs,
	S32 monitor, const struct PreviewAudioChoices *outputs, const char *path );
void PreviewAudio_Update( struct PreviewAudio *audio );
void PreviewAudio_Stop( struct PreviewAudio *audio );

#endif
