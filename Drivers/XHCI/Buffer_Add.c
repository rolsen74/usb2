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
XHCI_Buffer_Add ( struct USB2_HCDNode * hn, struct RealRequest * req, U32 offset, U32 length )
{
	struct USBBase * usbbase;
	struct XHCI_Buffer * buffer;
	struct XHCI_Buffer * tail;

	usbbase = hn->hn_USBBase;
	buffer = MEM_ALLOCVEC ( sizeof ( *buffer ), TRUE );

	if ( ! buffer )
	{
		return ( FALSE );
	}

	buffer->Memory = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

	if ( ! buffer->Memory )
	{
		MEM_FREEVEC ( buffer );

		return ( FALSE );
	}

	buffer->Physical = ( (struct Mem_FreeNode *)buffer->Memory )->mfn_Addr;
	MEM_SET ( buffer->Memory, 0, 4096 );
	buffer->Offset = offset;
	buffer->Length = length;
	if ( ! req->req_HCD.XHCI.Buffers )
	{
		req->req_HCD.XHCI.Buffers = buffer;
	}
	else
	{
		tail = req->req_HCD.XHCI.Buffers;

		while ( tail->Next )
		{
			tail = tail->Next;
		}

		tail->Next = buffer;
	}

	if ( req->req_Public.io_Command == CMD_WRITE )
	{
		MEM_COPY ( (U8 *)req->req_Public.io_Data + offset, buffer->Memory, length );
	}

	return ( TRUE );
}

// --
