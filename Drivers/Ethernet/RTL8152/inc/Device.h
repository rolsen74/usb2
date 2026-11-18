
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef RTL8152_DEVICE_H
#define RTL8152_DEVICE_H

#define ETHERNET_MTU        	1522
#define ETHERNET_PAYLOAD_MTU	1500
#define IS_MULTICAST(addr)		((((uint8 *)(addr))[0] & 0x01) != 0)
#define IS_BROADCAST(addr)		((addr)[0] == 0xff && (addr)[1] == 0xff && (addr)[2] == 0xff && \
							 (addr)[3] == 0xff && (addr)[4] == 0xff && (addr)[5] == 0xff)

/**************************************************************************/

struct devBase
{
	struct Device			dev_Base;
	uint16					dev_Pad;
	APTR					dev_SegList;
	struct SignalSemaphore	dev_GlobalLock;
	APTR					dev_Units[32];
};

/**************************************************************************/

struct AbortIOMessage
{
	struct Message		msg;
	struct IOSana2Req *	ioreq;
};

/**************************************************************************/

// This is what the back pointer ios2_BufferManagement actually points to.
struct BufferManagement
{
	APTR			bm_CopyToBuff;		// SANA2 callbacks
	APTR			bm_CopyFromBuff;
	struct Hook *	bm_PacketHook;
	struct Hook *	bm_CopyHook;
	Tag				bm_CopyToBuffTag;
	Tag				bm_CopyFromBuffTag;

};

/**************************************************************************/

// --

extern struct Library *NewlibBase;
extern struct Interface *INewlib;
extern struct TimerIFace *ITimer;

struct devBase *Device_Init( struct devBase *devBase, APTR seglist, struct ExecIFace *myIExec );
U32 Device_Obtain( struct DeviceManagerInterface *Self );
U32 Device_Release( struct DeviceManagerInterface *Self );
S32 Device_Open( struct DeviceManagerInterface *Self, struct IOSana2Req *ioreq, U32 unitnum, U32 flags );
APTR Device_Close( struct DeviceManagerInterface *Self, struct IOSana2Req *ioreq );
APTR Device_Expunge( struct DeviceManagerInterface *Self );
void Device_BeginIO( struct DeviceManagerInterface *Self, struct IOSana2Req *ioreq );
void Device_AbortIO( struct DeviceManagerInterface *Self, struct IOSana2Req *ioreq );
int OpenResources( struct devBase *devBase );
void CloseResources( struct devBase *devBase );

#endif

