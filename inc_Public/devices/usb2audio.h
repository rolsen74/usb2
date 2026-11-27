/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DEVICES_USB2AUDIO_H
#define DEVICES_USB2AUDIO_H

#include <devices/usb2.h>
#include <exec/io.h>
#include <interfaces/usb2media.h>

#define AUDIO_DEVICE_NAME "usb2audio.device"
#define AUDIO_DEFAULT_VAR "USB2-AudioDefault"
#define AUDIO_DEFAULT_AUTO "automatic"
#define AUDIO_CMD_QUERY 0x8000
// CMD_SET_RATE takes one native U32 in Hz while no read or write is active.
#define AUDIO_CMD_SET_RATE 0x8001
// GET_GAIN returns AudioGain. SET_GAIN takes one native S32 in 1/256 dB.
#define AUDIO_CMD_GET_GAIN 0x8002
#define AUDIO_CMD_SET_GAIN 0x8003
// CHECK_RATE validates a native U32 rate in Hz without changing the stream.
#define AUDIO_CMD_CHECK_RATE 0x8004
// QUEUE_WRITE can keep three writes outstanding from device revision 7.
// At revision 10, io_Offset returns the total detected USB gap in microseconds.
// CMD_RESET on an idle playback unit resets timing for a new series of writes.
#define AUDIO_CMD_QUEUE_WRITE 0x8005
#define AUDIO_THREE_WRITES_REVISION 7
#define AUDIO_OUTPUT_RECOVERY_REVISION 10
#define AUDIO_MAX_QUEUED_WRITES 3
// Query-unit command. io_Offset is AUDIO_ENDPOINT_VERSION, and io_Data points
// to a caller-owned array of AudioEndpoint records. io_Actual is the required
// byte count. A short buffer returns IOERR_BADLENGTH without copying records.
#define AUDIO_CMD_ENUM_ENDPOINTS 0x8006
// Like CMD_READ. On AUDIO_ERR_SHORT_READ, io_Offset is the duration in
// microseconds of a lost batch following the valid PCM, or zero when no
// whole batch was lost. io_Actual may be zero when io_Offset is nonzero.
#define AUDIO_CMD_READ_TIMED 0x8007
#define AUDIO_TIMED_READ_REVISION 9
#define AUDIO_ENDPOINT_VERSION 1
#define AUDIO_UNIT_ENDPOINT 0xfffffffeUL
#define AUDIO_UNIT_FIRST_OUTPUT 0xfffffffdUL
#define AUDIO_ENDPOINT_INPUT 1
#define AUDIO_ENDPOINT_OUTPUT 2
#define AUDIO_TRANSPORT_USB 1
#define AUDIO_ENDPOINT_BUSY 1
#define AUDIO_ENDPOINT_ID_SIZE 64
#define AUDIO_ENDPOINT_NAME_SIZE 64
#define AUDIO_ENDPOINT_MAX_RATES 82
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

struct AudioEndpoint
{
	U32 Direction;
	U32 Transport;
	U32 Flags;
	U32 LegacyUnit;
	struct AudioFormat DefaultFormat;
	U32 RateCount;
	U32 MinimumRate; // Continuous range when RateCount is zero.
	U32 MaximumRate;
	U32 Rates[AUDIO_ENDPOINT_MAX_RATES];
	char ID[AUDIO_ENDPOINT_ID_SIZE];
	char Name[AUDIO_ENDPOINT_NAME_SIZE];
};

// Open AUDIO_UNIT_ENDPOINT with this request, matching ID and Direction.
struct AudioEndpointRequest
{
	struct IOStdReq IO;
	U32 Version;
	U32 Direction;
	char ID[AUDIO_ENDPOINT_ID_SIZE];
};

#endif
