
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "EHCI.h"

// --

SEC_CODE S32
EHCI_Isochronous_Build ( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct USBBase *usbbase;
	struct USB2_EndPointNode *ep;
	struct EHCI_ITD **link;
	struct EHCI_ITD *itd;

	U32 packet;
	U32 mult;
	U32 capacity;
	U32 count;
	U32 step;
	U32 offset;
	U32 length;
	U32 physical;
	U32 in;
	U32 index;
	U32 size;
	U16 *lengths;

	usbbase = hn->hn_USBBase;
	ep = req->req_EndPoint;
	length = req->req_Public.io_Length;
	packet = ep->ep_MaxPacketSize & 0x7ff;
	mult = ( ( ep->ep_MaxPacketSize >> 11 ) & 3 ) + 1;
	in = req->req_Public.io_Command == CMD_READ;
	lengths = in ? NULL : req->req_Public.io_IsoPacketLengths;

	if ( req->req_Function->fkt_Speed != USBSPEED_High )
	{
		req->req_Public.io_Error = USB2Err_Host_IllegalSpeed;

		return ( TRUE );
	}

	if ( ( hn->hn_HCD.EHCI.IsoLinked ) || ( hn->hn_HCD.EHCI.IsoOrphan ) )
	{
		req->req_Public.io_Error = USB2Err_Host_NoBandWidth;

		return ( TRUE );
	}

	if ( ( ! length ) || ( ! req->req_Public.io_Data ) || ( ! packet ) || ( packet > 1024 ) ||
		( mult > 3 ) || ( ! ep->ep_Interval ) || ( ep->ep_Interval > 16 ) || ( ! hn->hn_HCD.EHCI.FrameList_Size ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	capacity = packet * mult;
	count = 1 + ( length - 1 ) / capacity;
	step = 1U << ( ep->ep_Interval - 1 );

	// Leave room for the initial scheduling lead and avoid revisiting a frame slot.
	if ( ( count > 256 ) || ( step > hn->hn_HCD.EHCI.FrameList_Size * 8 - 32 ) ||
		( count > ( hn->hn_HCD.EHCI.FrameList_Size * 8 - 32 ) / step ) ||
		( ( req->req_Public.io_IsoPacketLengths ) && ( req->req_Public.io_IsoPacketCount != count ) ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	link = &req->req_HCD.EHCI.IsoHead;

	for ( index = 0, offset = 0; index < count; index++, offset += capacity )
	{
		size = MIN( capacity, length - offset );

		if ( lengths )
		{
			if ( ( ! lengths[index] ) || ( lengths[index] > size ) )
			{
				req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

				return ( TRUE );
			}

			size = lengths[index];
		}

		itd = MEMORY_ALLOC( MEMID_HCD_4k, FALSE );

		if ( ! itd )
		{
			req->req_Public.io_Error = USB2Err_Stack_NoMemory;

			return ( TRUE );
		}

		physical = ( (struct Mem_FreeNode *) itd )->mfn_Addr;

		MEM_SET( itd, 0, sizeof( *itd ) );

		itd->Physical = physical;
		itd->Length = size;
		itd->Page[0] = LE_SWAP32( physical | ( ep->ep_Number << 8 ) | req->req_Function->fkt_Address );
		itd->Page[1] = LE_SWAP32( packet | ( in << 11 ) );
		itd->Page[2] = LE_SWAP32( mult );
		*link = itd;
		link = &itd->Next;

		if ( ! in )
		{
			MEM_COPY( (U8 *) req->req_Public.io_Data + offset, (U8 *) itd + EHCI_ITD_DATA, itd->Length );
		}
	}

	return ( FALSE );
}

// --
