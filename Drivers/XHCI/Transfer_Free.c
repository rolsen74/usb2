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

	usbbase = hn->hn_USBBase;
	index = 0;

	for ( buffer = req->req_HCD.XHCI.Buffers; buffer; buffer = next )
	{
		next = buffer->Next;

		if ( ( req->req_Public.io_Command == CMD_READ ) && ( ! req->req_Public.io_Error ) )
		{
			if ( req->req_EndPoint->ep_Type == EPATT_Type_Isochronous )
			{
				if ( req->req_Public.io_IsoPacketLengths )
				{
					req->req_Public.io_IsoPacketLengths[index] = buffer->Actual;
				}

				if ( buffer->Actual )
				{
					MEM_COPY ( buffer->Memory, (U8 *)req->req_Public.io_Data + buffer->Offset, buffer->Actual );
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

	MEM_SET ( &req->req_HCD.XHCI, 0, sizeof ( req->req_HCD.XHCI ) );
}

// --
