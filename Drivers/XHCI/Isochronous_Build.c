/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "XHCI.h"

// --

SEC_CODE S32
XHCI_Isochronous_Build ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct XHCI_Slot *		   slot;
	struct XHCI_Ring *		   ring;
	struct XHCI_Buffer *	   buffer;
	struct USB2_EndPointNode * ep;

	U32	  index;
	U32	  direction;
	U32	  count;
	U32	  packet;
	U32	  dci;
	U32	  length;
	U32	  extra;
	U32	  offset;
	U32	  size;
	U32	  capacity;
	U16 * lengths;

	ep		  = req->req_EndPoint;
	length	  = req->req_Public.io_Length;
	direction = req->req_Public.io_Command == CMD_READ;
	lengths	  = direction ? NULL : req->req_Public.io_IsoPacketLengths;
	packet	  = ep->ep_MaxPacketSize;
	extra	  = ep->ep_Transactions - 1;

	if ( ( ! length ) || ( ! req->req_Public.io_Data ) || ( ! packet ) || ( packet > 1024 )
		 || ( extra && ( ( extra > 2 ) || ( req->req_Function->fkt_Speed != USBSPEED_High ) ) ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	packet *= extra + 1;
	capacity = packet;

	if ( req->req_Function->fkt_Speed == USBSPEED_Super )
	{
		capacity = ep->ep_SS_IsoCapacity;

		if ( ! capacity )
		{
			req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

			return ( TRUE );
		}
	}

	count = 1 + ( length - 1 ) / packet;

	if ( ( count > 254 ) || ( req->req_Public.io_IsoPacketLengths && ( req->req_Public.io_IsoPacketCount != count ) ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	slot = XHCI_Slot_Find ( hn, req->req_Function->fkt_NotifyID );

	if ( ! slot )
	{
		req->req_Public.io_Error = USB2Err_Host_HostError;

		return ( TRUE );
	}

	dci = ep->ep_Number * 2 + ( direction ? 1 : 0 );

	if ( ! XHCI_Endpoint_Ensure ( hn, slot, ep, direction ) )
	{
		req->req_Public.io_Error = USB2Err_Host_HostError;

		return ( TRUE );
	}

	req->req_HCD.XHCI.Slot	   = slot;
	req->req_HCD.XHCI.EndPoint = dci;
	ring					   = &slot->EndPoints[dci];

	for ( index = 0, offset = 0; index < count; index++, offset += packet )
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

		if ( ! XHCI_Buffer_Add ( hn, req, offset, size ) )
		{
			req->req_Public.io_Error = USB2Err_Stack_NoMemory;

			return ( TRUE );
		}
	}

	for ( buffer = req->req_HCD.XHCI.Buffers; buffer; buffer = buffer->Next )
	{
		buffer->TRB_Physical	   = XHCI_Ring_Queue ( ring, buffer->Physical, 0, buffer->Length,
													   XHCI_TRB_TYPE ( XHCI_TRB_ISOCH ) | XHCI_TRB_SIA | XHCI_TRB_IOC );
		req->req_HCD.XHCI.Last_TRB = buffer->TRB_Physical;
	}

	return ( FALSE );
}

// --
