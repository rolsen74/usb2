
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

_Static_assert ( offsetof ( struct EHCI_ITD, Split.Status ) == 0x0c, "siTD status offset" );
_Static_assert ( offsetof ( struct EHCI_ITD, Split.BackLink ) == 0x18, "siTD back link offset" );
_Static_assert ( sizeof ( struct EHCI_ITD ) <= EHCI_ITD_DATA, "isochronous descriptor overlaps DMA payload" );

#define EHCI_ISO_MIN_LEAD 9U
#define EHCI_ISO_LEAD	  16U

// --

static SEC_CODE void
BuildSplitPacket ( struct EHCI_ITD * itd, U32 endpoint, U32 in, U32 start, U32 in_schedule )
{
	U32 splits;
	U32 schedule;
	U32 position;
	U32 count;

	position = 0;
	count	 = 0;

	if ( in )
	{
		schedule = in_schedule;
	}
	else
	{
		// OUT carries up to 188 bytes in each consecutive start-split.
		splits	 = ( itd->Length + EHCI_SITD_PAYLOAD - 1 ) / EHCI_SITD_PAYLOAD;
		schedule = ( ( 1U << splits ) - 1 ) << start;
		position = ( splits > 1 ) ? EHCI_SITD_TP_BEGIN : 0;
		count	 = splits;
	}

	itd->Split.Endpoint	 = LE_SWAP32 ( endpoint );
	itd->Split.Schedule	 = LE_SWAP32 ( schedule );
	itd->Split.Buffer[0] = LE_SWAP32 ( itd->Physical | EHCI_ITD_DATA );
	itd->Split.Buffer[1] = LE_SWAP32 ( itd->Physical | position | count );
	itd->Split.BackLink	 = EHCI_NULL;
	itd->Mask			 = ( schedule | ( schedule >> 8 ) ) & 0xff;
}

// --

static SEC_CODE U32
SplitInterruptMask ( struct USB2_HCDNode * hn, U32 frame, U32 hub )
{
	struct EHCI_QH * qh;

	U32 slot;
	U32 caps;
	U32 mask;

	slot = EHCI_Reverse_Bits ( frame, EHCI_IPOLLRATES - 1 );
	slot = EHCI_IQHIDX ( EHCI_IPOLLRATES, slot );
	mask = 0;

	for ( qh = hn->hn_HCD.EHCI.FrameList_ISlots[slot]; qh; qh = qh->pqh_virt_next )
	{
		caps = LE_SWAP32 ( qh->qh_ep2 );

		if ( EHCI_QH_GET_HUBA ( caps ) == hub )
		{
			mask |= EHCI_QH_GET_SMASK ( caps ) | EHCI_QH_GET_CMASK ( caps );
		}
	}

	// Conservatively reserve the TT through its last scheduled complete-split.
	while ( mask & ( mask - 1 ) )
	{
		mask &= mask - 1;
	}

	return ( mask ? ( mask << 1 ) - 1 : 0 );
}

// --

static SEC_CODE S32
SchedulePackets ( struct USB2_HCDNode * hn, struct RealRequest * req, U32 count, U32 step, U32 split, U32 lead, U32 splitStart )
{
	struct EHCI_ITD *	 itd;
	struct EHCI_ITD *	 queued;
	struct RealRequest * active;

	U32 start;
	U32 microframe;
	U32 phase;
	U32 phases;
	U32 shift;
	U32 frame;
	U32 mask;
	U32 index;
	U32 usecs[8];
	U32 overlap;
	U32 current;
	U32 distance;
	U32 horizon;
	U32 latest;
	U32 downstream;
	S32 splitOut;
	S32 checkInterrupts;
	S32 fits;

	current	 = PCI_READLONG ( hn->hn_HCD.EHCI.CapLength + EHCI_FRINDEX );
	start	 = current + lead;
	phases	 = MIN ( step, 8 );
	latest	 = lead + 7;
	horizon	 = hn->hn_HCD.EHCI.FrameList_Size * 8;
	active	 = req->req_EndPoint->ep_Request_Active;
	splitOut = split && ( req->req_Public.io_Command == CMD_WRITE );

	if ( split )
	{
		start = ( start + 7 ) & ~7U;
		step *= 8;
		phases = splitOut ? 8 : 1;
	}

	if ( active )
	{
		distance = ( active->req_HCD.EHCI.IsoNextFrame - current ) & EHCI_FRINDEX_MASK;

		if ( ( distance >= EHCI_ISO_MIN_LEAD ) && ( distance < horizon ) )
		{
			start  = current + distance;
			phases = splitOut ? 8 : 1;
			latest = horizon - 1 - ( count - 1 ) * step - ( split ? 7 : 0 );
		}
		else if ( splitOut )
		{
			// Queue the continuation if its next frame cannot safely be linked.
			return ( FALSE );
		}
	}

	for ( phase = 0; phase < phases; phase++ )
	{
		microframe		= split ? start : start + phase;
		checkInterrupts = splitOut && ( phase < 7 );
		// Start as early as the TT schedule allows. Keep the legacy phase as the fallback.
		shift = checkInterrupts ? phase : ( splitOut ? splitStart : 0 );

		if ( ( ( microframe - current ) & EHCI_FRINDEX_MASK ) > latest )
		{
			break;
		}

		for ( itd = req->req_HCD.EHCI.IsoHead; itd; itd = itd->Next, microframe += step )
		{
			frame = ( microframe >> 3 ) & ( hn->hn_HCD.EHCI.FrameList_Size - 1 );
			mask  = split ? (U32)itd->Mask << shift : 1U << ( microframe & 7 );
			fits  = TRUE;

			if ( ( splitOut ) && ( mask & ~0x7fU ) )
			{
				break;
			}

			if ( checkInterrupts )
			{
				// Allow for worst-case bit stuffing and protocol overhead at full speed.
				downstream = ( itd->Length * 7 + 8 ) / 9 + 8;

				if ( ( downstream > ( 6 - shift ) * 125 + 30 ) || ( mask & SplitInterruptMask ( hn, frame, itd->Hub ) ) )
				{
					break;
				}
			}

			for ( index = 0; index < 8; index++ )
			{
				usecs[index] = itd->Usecs;
			}

			for ( queued = hn->hn_HCD.EHCI.IsoFrames[frame]; queued; queued = queued->FrameNext )
			{
				// Treat a hub as a single TT. Its full-speed transfers share a frame.
				if ( ( itd->Hub ) && ( queued->Hub == itd->Hub ) )
				{
					fits = FALSE;
					break;
				}

				overlap = mask & queued->Mask;

				for ( index = 0; overlap; index++, overlap >>= 1 )
				{
					if ( overlap & 1 )
					{
						usecs[index] += queued->Usecs;

						if ( usecs[index] > EHCI_ISO_USECS )
						{
							fits = FALSE;
							break;
						}
					}
				}

				if ( ! fits )
				{
					break;
				}
			}

			if ( ! fits )
			{
				break;
			}

			itd->Frame		= frame;
			itd->Microframe = microframe & 7;
			if ( ! splitOut )
			{
				itd->Mask = mask;
			}
		}

		if ( ! itd )
		{
			distance = ( start + ( split ? 0 : phase ) - PCI_READLONG ( hn->hn_HCD.EHCI.CapLength + EHCI_FRINDEX ) )
					   & EHCI_FRINDEX_MASK;

			// Frame caching can begin before the first transaction's microframe.
			if ( ( distance >= EHCI_ISO_MIN_LEAD ) && ( distance <= latest ) )
			{
				if ( splitOut )
				{
					for ( itd = req->req_HCD.EHCI.IsoHead; itd; itd = itd->Next )
					{
						itd->Mask <<= shift;
						itd->Split.Schedule = LE_SWAP32 ( (U32)itd->Mask );
					}
				}

				req->req_HCD.EHCI.IsoNextFrame = microframe & EHCI_FRINDEX_MASK;
				req->req_HCD.EHCI.IsoLastFrame = ( microframe - step + ( split ? 7 : 0 ) ) & EHCI_FRINDEX_MASK;

				return ( TRUE );
			}

			break;
		}
	}

	return ( FALSE );
}

// --

SEC_CODE S32
EHCI_Isochronous_Build ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct USBBase *		   usbbase;
	struct USB2_EndPointNode * ep;
	struct EHCI_ITD **		   link;
	struct EHCI_ITD *		   itd;

	U32	  packet;
	U32	  mult;
	U32	  capacity;
	U32	  count;
	U32	  step;
	U32	  offset;
	U32	  length;
	U32	  physical;
	U32	  in;
	U32	  index;
	U32	  size;
	U32	  split;
	U32	  hub;
	U32	  port;
	U32	  endpoint;
	U32	  splits;
	U32	  start;
	U32	  in_schedule;
	U32	  budget;
	U32	  usecs;
	U16 * lengths;

	usbbase		= hn->hn_USBBase;
	ep			= req->req_EndPoint;
	length		= req->req_Public.io_Length;
	packet		= ep->ep_MaxPacketSize;
	mult		= ep->ep_Transactions;
	in			= req->req_Public.io_Command == CMD_READ;
	lengths		= in ? NULL : req->req_Public.io_IsoPacketLengths;
	split		= req->req_Function->fkt_Speed == USBSPEED_Full;
	endpoint	= 0;
	start		= 0;
	in_schedule = 0;
	hub			= 0;

	if ( ( ! split ) && ( req->req_Function->fkt_Speed != USBSPEED_High ) )
	{
		req->req_Public.io_Error = USB2Err_Host_IllegalSpeed;

		return ( TRUE );
	}

	if ( hn->hn_HCD.EHCI.IsoOrphan )
	{
		req->req_Public.io_Error = USB2Err_Host_NoBandWidth;

		return ( TRUE );
	}

	if ( ( ! length ) || ( ! req->req_Public.io_Data ) || ( ! packet ) || ( packet > 1024 ) || ( mult > 3 )
		 || ( ! ep->ep_Interval ) || ( ep->ep_Interval > 16 ) || ( hn->hn_HCD.EHCI.FrameList_Size <= 5 )
		 || ( ( split ) && ( ( mult != 1 ) || ( packet > 1023 ) ) ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	capacity = packet * mult;
	count	 = 1 + ( length - 1 ) / capacity;
	step	 = 1U << ( ep->ep_Interval - 1 );
	budget	 = hn->hn_HCD.EHCI.FrameList_Size * 8 - EHCI_ISO_RETRY_LEAD;
	// Worst-case bit stuffing plus protocol overhead for each high-speed transaction.
	usecs = mult * ( ( ( packet * 7 + 5 ) / 6 + 64 + 59 ) / 60 );

	if ( split )
	{
		if ( ! EHCI_Get_TT ( req->req_Function, &hub, &port ) )
		{
			req->req_Public.io_Error = USB2Err_Host_IllegalSpeed;

			return ( TRUE );
		}

		endpoint = req->req_Function->fkt_Address | ( ep->ep_Number << 8 ) | ( hub << 16 ) | ( port << 24 ) | ( in << 31 );
		splits	 = ( packet + EHCI_SITD_PAYLOAD - 1 ) / EHCI_SITD_PAYLOAD;

		if ( in )
		{
			// Allow two extra complete-splits without spanning the frame boundary.
			if ( splits > 4 )
			{
				req->req_Public.io_Error = USB2Err_Host_NoBandWidth;

				return ( TRUE );
			}

			start		= ( splits < 4 ) ? 1 : 0;
			in_schedule = ( 1U << start ) | ( ( ( 1U << ( splits + 2 ) ) - 1 ) << ( 10 + start ) );
		}
		else
		{
			// Preserve the original phase if no phase clears the conservative TT checks.
			start = ( splits < 6 ) ? 1 : 0;
		}

		// Full-speed intervals count frames. Leave room to align the start.
		budget = hn->hn_HCD.EHCI.FrameList_Size - EHCI_ISO_RETRY_LEAD / 8 - 1;
		usecs  = 8;
	}

	// Leave room for the initial scheduling lead and avoid revisiting a frame slot.
	if ( ( count > EHCI_ISO_MAX_PACKETS ) || ( count > budget / step )
		 || ( ( req->req_Public.io_IsoPacketLengths ) && ( req->req_Public.io_IsoPacketCount != count ) ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	link = &req->req_HCD.EHCI.IsoHead;

	EHCI_Isochronous_Reap ( hn, FALSE );

	for ( index = 0, offset = 0; index < count; index++, offset += capacity )
	{
		size = MIN ( capacity, length - offset );

		if ( lengths )
		{
			if ( ( ! lengths[index] ) || ( lengths[index] > size ) )
			{
				req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

				return ( TRUE );
			}

			size = lengths[index];
		}

		itd = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

		if ( ! itd )
		{
			req->req_Public.io_Error = USB2Err_Stack_NoMemory;

			return ( TRUE );
		}

		physical = ( (struct Mem_FreeNode *)itd )->mfn_Addr;

		MEM_SET ( itd, 0, sizeof ( *itd ) );

		itd->Physical = physical;
		itd->Length	  = size;
		itd->Usecs	  = usecs;
		itd->Hub	  = hub;

		if ( split )
		{
			BuildSplitPacket ( itd, endpoint, in, in ? start : 0, in_schedule );
		}
		else
		{
			itd->Page[0] = LE_SWAP32 ( physical | ( ep->ep_Number << 8 ) | req->req_Function->fkt_Address );
			itd->Page[1] = LE_SWAP32 ( packet | ( in << 11 ) );
			itd->Page[2] = LE_SWAP32 ( mult );
		}

		*link = itd;
		link  = &itd->Next;

		if ( ! in )
		{
			MEM_COPY ( (U8 *)req->req_Public.io_Data + offset, (U8 *)itd + EHCI_ITD_DATA, itd->Length );
		}
	}

	if ( ( ! SchedulePackets ( hn, req, count, step, split, EHCI_ISO_LEAD, start ) )
		 && ( ! SchedulePackets ( hn, req, count, step, split, EHCI_ISO_RETRY_LEAD, start ) ) )
	{
		req->req_Public.io_Error = USB2Err_Host_NoBandWidth;

		return ( TRUE );
	}

	return ( FALSE );
}

// --
