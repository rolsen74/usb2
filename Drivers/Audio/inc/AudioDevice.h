/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DRIVERS_AUDIO_DEVICE_H
#define DRIVERS_AUDIO_DEVICE_H

#define AUDIO_DEVICE_NAME "usb2audio.device"
#define AUDIO_CMD_QUERY 0x8000
// CMD_SET_RATE takes one native U32 in Hz while no read or write is active.
#define AUDIO_CMD_SET_RATE 0x8001
#define AUDIO_ENCODING_PCM_S16LE 1
#define AUDIO_PLAYBACK_UNIT_BASE 16

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

#endif
