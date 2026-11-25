/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DRIVERS_AUDIO_DEVICE_H
#define DRIVERS_AUDIO_DEVICE_H

#define AUDIO_DEVICE_NAME "usb2audio.device"
#define AUDIO_DEVICE_VERSION 1
#define AUDIO_DEVICE_REVISION 2
#define AUDIO_DEVICE_ID AUDIO_DEVICE_NAME " 1.2 (25.11.2026)"
#define AUDIO_CMD_QUERY 0x8000
// CMD_SET_RATE takes one native U32 in Hz while no read or write is active.
#define AUDIO_CMD_SET_RATE 0x8001
// GET_GAIN returns AudioGain. SET_GAIN takes one native S32 in 1/256 dB.
#define AUDIO_CMD_GET_GAIN 0x8002
#define AUDIO_CMD_SET_GAIN 0x8003
// CHECK_RATE validates a native U32 rate in Hz without changing the stream.
#define AUDIO_CMD_CHECK_RATE 0x8004
// QUEUE_WRITE accepts a second write while the first is active.
#define AUDIO_CMD_QUEUE_WRITE 0x8005
#define AUDIO_ENCODING_PCM_S16LE 1
#define AUDIO_PLAYBACK_UNIT_BASE 16
#define AUDIO_MAX_UNITS 32
#define AUDIO_ERR_NO_SAMPLES (-40)
#define AUDIO_ERR_RATE_SETUP (-41)
#define AUDIO_ERR_NO_PACKETS (-42)
#define AUDIO_ERR_SHORT_WRITE (-43)
#define AUDIO_ERR_SHORT_READ (-44)

struct AudioFormat
{
	U32 Encoding;
	U32 SampleRate;
	U32 Channels;
	U32 BitsPerSample;
	U32 PacketBytes;
	U32 VendorID;
	U32 DeviceID;
};

struct AudioGain
{
	S32 Minimum;
	S32 Maximum;
	S32 Current;
	S32 Step;
};

#endif
