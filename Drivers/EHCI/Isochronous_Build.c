
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "EHCI.h"
#include <stddef.h>

// --

_Static_assert( offsetof( struct EHCI_ITD, Split.Status ) == 0x0c, "siTD status offset" );
_Static_assert( offsetof( struct EHCI_ITD, Split.BackLink ) == 0x18, "siTD back link offset" );
_Static_assert( sizeof( struct EHCI_ITD ) <= EHCI_ITD_DATA, "isochronous descriptor overlaps DMA payload" );

// --

static SEC_CODE void
BuildSplitPacket( struct EHCI_ITD *itd, U32 endpoint, U32 in, U32 start, U32 in_schedule )
{
	U32 splits;
	U32 schedule;
	U32 position;
	U32 count;

	position = 0;
	count = 0;

	if ( in )
	{
		schedule = in_schedule;
	}
	else
	{
		// OUT carries up to 188 bytes in each consecutive start-split.
		splits = ( itd->Length + EHCI_SITD_PAYLOAD - 1 ) / EHCI_SITD_PAYLOAD;
		schedule = ( ( 1U << splits ) - 1 ) << start;
		position = ( splits > 1 ) ? EHCI_SITD_TP_BEGIN : 0;
		count = splits;
	}

	itd->Split.Endpoint = LE_SWAP32( endpoint );
	itd->Split.Schedule = LE_SWAP32( schedule );
	itd->Split.Buffer[0] = LE_SWAP32( itd->Physical | EHCI_ITD_DATA );
	itd->Split.Buffer[1] = LE_SWAP32( itd->Physical | position | count );
	itd->Split.BackLink = EHCI_NULL;
}

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
	U32 split;
	U32 hub;
	U32 port;
	U32 endpoint;
	U32 splits;
	U32 start;
	U32 in_schedule;
	U32 budget;
	U16 *lengths;

	usbbase = hn->hn_USBBase;
	ep = req->req_EndPoint;
	length = req->req_Public.io_Length;
	packet = ep->ep_MaxPacketSize & 0x7ff;
	mult = ( ( ep->ep_MaxPacketSize >> 11 ) & 3 ) + 1;
	in = req->req_Public.io_Command == CMD_READ;
	lengths = in ? NULL : req->req_Public.io_IsoPacketLengths;
	split = req->req_Function->fkt_Speed == USBSPEED_Full;
	endpoint = 0;
	start = 0;
	in_schedule = 0;

	if ( ( ! split ) && ( req->req_Function->fkt_Speed != USBSPEED_High ) )
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
		( mult > 3 ) || ( ! ep->ep_Interval ) || ( ep->ep_Interval > 16 ) || ( hn->hn_HCD.EHCI.FrameList_Size <= 5 ) ||
		( ( split ) && ( ( mult != 1 ) || ( packet > 1023 ) ) ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	capacity = packet * mult;
	count = 1 + ( length - 1 ) / capacity;
	step = 1U << ( ep->ep_Interval - 1 );
	budget = hn->hn_HCD.EHCI.FrameList_Size * 8 - 32;

	if ( split )
	{
		if ( ! EHCI_Get_TT( req->req_Function, &hub, &port ) )
		{
			req->req_Public.io_Error = USB2Err_Host_IllegalSpeed;

			return ( TRUE );
		}

		endpoint = req->req_Function->fkt_Address | ( ep->ep_Number << 8 ) | ( hub << 16 ) | ( port << 24 ) | ( in << 31 );
		splits = ( packet + EHCI_SITD_PAYLOAD - 1 ) / EHCI_SITD_PAYLOAD;

		if ( in )
		{
			// Allow two extra complete-splits without spanning the frame boundary.
			if ( splits > 4 )
			{
				req->req_Public.io_Error = USB2Err_Host_NoBandWidth;

				return ( TRUE );
			}

			start = ( splits < 4 ) ? 1 : 0;
			in_schedule = ( 1U << start ) | ( ( ( 1U << ( splits + 2 ) ) - 1 ) << ( 10 + start ) );
		}
		else
		{
			// Leave microframe zero to split interrupt QHs when there is room.
			start = ( splits < 6 ) ? 1 : 0;
		}

		// Full-speed intervals count frames. Leave room to align the start.
		budget = hn->hn_HCD.EHCI.FrameList_Size - 5;
	}

	// Leave room for the initial scheduling lead and avoid revisiting a frame slot.
	if ( ( count > 256 ) || ( count > budget / step ) ||
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

		if ( split )
		{
			BuildSplitPacket( itd, endpoint, in, start, in_schedule );
		}
		else
		{
			itd->Page[0] = LE_SWAP32( physical | ( ep->ep_Number << 8 ) | req->req_Function->fkt_Address );
			itd->Page[1] = LE_SWAP32( packet | ( in << 11 ) );
			itd->Page[2] = LE_SWAP32( mult );
		}

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
