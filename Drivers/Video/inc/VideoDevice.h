/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DRIVERS_VIDEO_DEVICE_H
#define DRIVERS_VIDEO_DEVICE_H

#define VIDEO_DEVICE_NAME "usb2video.device"
#define VIDEO_CMD_QUERY 0x8000
// io_Offset selects a mode. GET_MODE returns a VideoFormat for modes 1 and up.
// SET_MODE takes no buffer. Mode 0 restores the default camera mode.
#define VIDEO_CMD_GET_MODE 0x8001
#define VIDEO_CMD_SET_MODE 0x8002
// Discard buffered packets and stop USB reads. Takes no buffer.
#define VIDEO_CMD_STOP 0x8003
#define VIDEO_FORMAT_MJPEG 1
#define VIDEO_FORMAT_YUY2 2

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
