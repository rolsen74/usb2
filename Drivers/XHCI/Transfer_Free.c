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

SEC_CODE void
XHCI_Transfer_Free ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct USBBase * usbbase;
	struct XHCI_Buffer * buffer;
	struct XHCI_Buffer * next;

	U32 index;
	U32 offset;
	S32 aggregate_loss;
	S32 lost;

	usbbase = hn->hn_USBBase;
	index = 0;
	offset = 0;
	aggregate_loss = ( ! req->req_Public.io_Error ) && ( req->req_EndPoint->ep_Type == EPATT_Type_Isochronous ) &&
		( ( req->req_Public.io_Command != CMD_READ ) || ( ! req->req_Public.io_IsoPacketLengths ) );
	lost = req->req_HCD.XHCI.Iso_Lost;

	for ( buffer = req->req_HCD.XHCI.Buffers; buffer; buffer = next )
	{
		next = buffer->Next;

		if ( ( aggregate_loss ) && ( ! buffer->Done ) )
		{
			lost = TRUE;
		}

		if ( ( req->req_Public.io_Command == CMD_READ ) && ( ! req->req_Public.io_Error ) )
		{
			if ( req->req_EndPoint->ep_Type == EPATT_Type_Isochronous )
			{
				if ( req->req_Public.io_IsoPacketLengths )
				{
					req->req_Public.io_IsoPacketLengths[index] =
						( ( ! buffer->Done ) || ( buffer->Failed ) ) ? USB2_ISO_PACKET_ERROR : buffer->Actual;
					offset = buffer->Offset;
				}

				if ( buffer->Actual )
				{
					MEM_COPY ( buffer->Memory, (U8 *)req->req_Public.io_Data + offset, buffer->Actual );

					offset += buffer->Actual;
				}
			}
			else if ( buffer->Offset < req->req_HCD.XHCI.Actual )
			{
				MEM_COPY ( buffer->Memory, (U8 *)req->req_Public.io_Data + buffer->Offset,
					MIN ( buffer->Length, req->req_HCD.XHCI.Actual - buffer->Offset ) );
			}
		}

		MEMORY_FREE ( MEMID_HCD_4k, buffer->Memory, buffer->Physical );
		MEM_FREEVEC ( buffer );
		index++;
	}

	if ( ( aggregate_loss ) && ( lost ) )
	{
		req->req_Public.io_Error = USB2Err_Host_HostError;
	}

	MEM_SET ( &req->req_HCD.XHCI, 0, sizeof ( req->req_HCD.XHCI ) );
}

// --
