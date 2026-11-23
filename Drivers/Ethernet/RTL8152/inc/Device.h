
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

struct SanaRequest
{
	struct Node Node;
	struct IOSana2Req *Request;
	U8 Started;
	U8 Cancelled;
	U8 AbortHandled;
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

#endif
