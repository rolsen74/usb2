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
XHCI_Interrupt_Build ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct XHCI_State *		   state;
	struct XHCI_Slot *		   slot;
	struct XHCI_Ring *		   ring;
	struct XHCI_Buffer *	   buffer;
	struct USB2_EndPointNode * ep;

	U32 index;
	U32 direction;
	U32 count;
	U32 control;
	U32 dci;
	U32 length;
	U32 packet;
	U32 td_size;

	state	  = hn->hn_HCD.XHCI.State;
	ep		  = req->req_EndPoint;
	length	  = req->req_Public.io_Length;
	packet	  = ep->ep_MaxPacketSize;
	direction = req->req_Public.io_Command == CMD_READ;

	if ( ( length > 254 * 4096 ) || ( ( length ) && ( ! req->req_Public.io_Data ) ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	slot = NULL;

	for ( index = 1; index <= state->Max_Slots; index++ )
	{
		if ( ( state->Slots[index] ) && ( state->Slots[index]->Function == req->req_Function ) )
		{
			slot = state->Slots[index];
			break;
		}
	}

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

	for ( count = 0; count < length; count += 4096 )
	{
		if ( ! XHCI_Buffer_Add ( hn, req, count, MIN ( 4096, length - count ) ) )
		{
			req->req_Public.io_Error = USB2Err_Stack_NoMemory;

			return ( TRUE );
		}
	}

	if ( ! length )
	{
		req->req_HCD.XHCI.Last_TRB = XHCI_Ring_Queue ( ring, 0, 0, 0, XHCI_TRB_TYPE ( XHCI_TRB_NORMAL ) | XHCI_TRB_IOC );
	}

	for ( buffer = req->req_HCD.XHCI.Buffers; buffer; buffer = buffer->Next )
	{
		td_size = buffer->Next ? MIN ( 31, ( length + packet - 1 ) / packet - ( buffer->Offset + buffer->Length ) / packet ) : 0;
		control = XHCI_TRB_TYPE ( XHCI_TRB_NORMAL ) | ( direction ? XHCI_TRB_ISP : 0 );

		if ( buffer->Next )
		{
			control |= XHCI_TRB_CHAIN;
		}
		else
		{
			control |= XHCI_TRB_IOC;
		}

		buffer->TRB_Physical	   = XHCI_Ring_Queue ( ring, buffer->Physical, 0, buffer->Length | ( td_size << 17 ), control );
		req->req_HCD.XHCI.Last_TRB = buffer->TRB_Physical;
	}

	return ( FALSE );
}

// --
