
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "OHCI.h"

// --

SEC_CODE S32
OHCI_Isochronous_Build ( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct USBBase *usbbase;
	struct USB2_EndPointNode *ep;
	struct OHCI_ED *ed;
	struct OHCI_ITD **link;
	struct OHCI_ITD *itd;

	U32 count;
	U32 length;
	U32 packet;
	U32 offset;
	U32 size;
	U32 physical;
	U32 index;
	U32 step;
	S32 in;

	usbbase = hn->hn_USBBase;
	ep = req->req_EndPoint;
	length = req->req_Public.io_Length;
	packet = ep->ep_MaxPacketSize;
	in = req->req_Public.io_Command == CMD_READ;

	if ( req->req_Function->fkt_Speed != USBSPEED_Full )
	{
		req->req_Public.io_Error = USB2Err_Host_IllegalSpeed;

		return ( TRUE );
	}

	if ( hn->hn_HCD.OHCI.Isoc_Head->ped_virt_next )
	{
		req->req_Public.io_Error = USB2Err_Host_NoBandWidth;

		return ( TRUE );
	}

	if ( ( ! length ) || ( ! req->req_Public.io_Data ) || ( ! packet ) || ( packet > 1023 ) ||
		( ! ep->ep_Interval ) || ( ep->ep_Interval > 16 ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	count = 1 + ( length - 1 ) / packet;
	step = 1U << ( ep->ep_Interval - 1 );

	// Limit the schedule to less than half the 16-bit frame number range.
	if ( ( count > 256 ) || ( count > 32764 / step ) ||
		( ( req->req_Public.io_IsoPacketLengths ) && ( req->req_Public.io_IsoPacketCount != count ) ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	ed = OHCI_Get_EDBuffer( hn, req );

	if ( ! ed )
	{
		req->req_Public.io_Error = USB2Err_Stack_NoMemory;

		return ( TRUE );
	}

	ed->ed_flags = LE_SWAP32( LE_SWAP32( ed->ed_flags ) | OHCI_ED_FORMAT_ISO |
		( in ? OHCI_ED_DIR_IN : OHCI_ED_DIR_OUT ) );
	link = &req->req_HCD.OHCI.IsoHead;
	offset = 0;

	for ( index = 0; index <= count; index++ )
	{
		itd = MEMORY_ALLOC( MEMID_HCD_4k, FALSE );

		if ( ! itd )
		{
			req->req_Public.io_Error = USB2Err_Stack_NoMemory;

			return ( TRUE );
		}

		physical = ( (struct Mem_FreeNode *) itd )->mfn_Addr;

		MEM_SET( itd, 0, sizeof( *itd ) );

		itd->Physical = physical;
		*link = itd;
		link = &itd->Next;

		if ( index == count )
		{
			// OHCI requires a dummy tail which is never processed.
			ed->ed_tailp = LE_SWAP32( physical );
			break;
		}

		size = MIN( packet, length - offset );
		itd->Length = size;
		itd->itd_flags = LE_SWAP32( OHCI_TD_NOCC | ( index + 1 == count ? 0 : OHCI_TD_NOINTR ) );
		itd->itd_bp0 = LE_SWAP32( physical );
		itd->itd_be = LE_SWAP32( physical + OHCI_ITD_DATA + size - 1 );
		itd->itd_offset[0] = LE_SWAP16( 0xe000U | OHCI_ITD_DATA );

		if ( ! in )
		{
			MEM_COPY( (U8 *) req->req_Public.io_Data + offset, (U8 *) itd + OHCI_ITD_DATA, size );
		}

		offset += size;
	}

	ed->ed_headp = LE_SWAP32( req->req_HCD.OHCI.IsoHead->Physical );

	return ( FALSE );
}

// --
