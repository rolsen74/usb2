
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef STACK_MEDIADEVICE_H
#define STACK_MEDIADEVICE_H

#include "usb2_all.h"
#include <interfaces/usb2media.h>

struct USB2MediaDevice
{
	struct Library Library;
	struct ExecIFace *IExec;
	struct SignalSemaphore Lock;
	struct DeviceManagerInterface *Backend;
	const U16 *Commands;
};

// --

SEC_CODE S32 MediaDevice_Create( struct USBBase *usbbase );
SEC_CODE BOOL MediaDevice_Register( struct USB2MediaIFace *Self, struct Device *backend );
SEC_CODE struct DeviceManagerInterface *MediaDevice_Backend( struct USB2MediaDevice *base );
SEC_CODE void MediaDevice_Open( struct DeviceManagerInterface *Self, struct IORequest *request, U32 number, U32 flags );
SEC_CODE PTR MediaDevice_Close( struct DeviceManagerInterface *Self, struct IORequest *request );
SEC_CODE void MediaDevice_BeginIO( struct DeviceManagerInterface *Self, struct IORequest *request );
SEC_CODE void MediaDevice_AbortIO( struct DeviceManagerInterface *Self, struct IORequest *request );

#endif
