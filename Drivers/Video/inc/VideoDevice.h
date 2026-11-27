/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DRIVERS_VIDEO_DEVICE_H
#define DRIVERS_VIDEO_DEVICE_H

#define VIDEO_DEVICE_NAME "usb2video.device"
#define VIDEO_DEVICE_VERSION 1
#define VIDEO_DEVICE_REVISION 10
#define VIDEO_UNIT_LIST_REVISION 5
#define VIDEO_RECONNECT_REVISION 9
#define VIDEO_DEVICE_ID VIDEO_DEVICE_NAME " 1.10 (27.11.2026)"
#define VIDEO_FRAME_TIMEOUT_SECONDS 10
#define VIDEO_ERR_FRAME_TIMEOUT (-45)
#define VIDEO_MAX_UNITS 16
#define VIDEO_CMD_QUERY 0x8000
// io_Offset selects a mode. Mode 0 selects the default negotiated at attachment.
// GET_MODE returns a VideoFormat. SET_MODE takes no buffer.
#define VIDEO_CMD_GET_MODE 0x8001
#define VIDEO_CMD_SET_MODE 0x8002
// Stop capture and keep the selected mode for the next CMD_READ. Takes no buffer.
#define VIDEO_CMD_STOP 0x8003
// Query the required bytes per isochronous USB buffer in io_Actual. Bulk modes return zero.
#define VIDEO_CMD_GET_ISO_BUFFER_SIZE 0x8004
// Register three caller-owned USB buffers before capture. The original two-buffer
// request length is also accepted. Keep the buffers until STOP replies.
#define VIDEO_CMD_SET_ISO_BUFFERS 0x8005
// Returns the current or most recent capture counters in a VideoCaptureStats.
// The original five-counter buffer is also accepted.
#define VIDEO_CMD_GET_CAPTURE_STATS 0x8006
#define VIDEO_ISO_BUFFERS 3
#define VIDEO_FORMAT_MJPEG 1
#define VIDEO_FORMAT_YUY2 2

struct VideoIsoBuffers
{
	U32 Size;
	PTR Buffer[VIDEO_ISO_BUFFERS];
};

struct VideoCaptureStats
{
	U32 DeliveredFrames;
	U32 IsoBatches;
	U32 IsoBatchGaps;
	U32 IsoPacketErrors;
	U32 FrameLengthErrors;
	U32 PayloadFrameStarts;
	U32 InvalidPayloadHeaders;
	U32 PayloadErrorFlags;
	U32 FrameInterval;
	U32 LinkSpeed; // USBSPEED value, or ~0U if unavailable.
	U32 IsoServiceUS;
	U32 IsoBatchPackets;
};

struct VideoFormat
{
	U32 Format;
	U32 Width;
	U32 Height;
	U32 MaxFrameSize;
	U32 FrameInterval;
	U32 VendorID;
	U32 DeviceID;
};

#endif
