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
