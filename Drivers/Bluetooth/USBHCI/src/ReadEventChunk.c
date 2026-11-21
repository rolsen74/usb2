
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
BTUSB_QueueEvent( struct BTUSB *usb, enum BTUSBEventRead kind )
{
	struct USB2_IORequest *request;

	if ( usb->EventPending != BTUSB_EVENT_IDLE )
	{
		return;
	}

	request = usb->Events->IORequests[0];
	request->io_Command = CMD_READ;
	request->io_Data = usb->Events->Buffers[0];
	request->io_Length = usb->Events->BufferSize;
	usb->EventPending = kind;

	SendIO( (PTR) request );
}

// --

S32
BTUSB_ReadEventChunk( PTR context, const U8 **data, U32 *length )
{
	struct BTUSB *usb;
	struct USB2_IORequest *request;
	enum BTUSBEventRead kind;

	S32 error;

	usb = context;
	request = usb->Events->IORequests[0];

	// A packet-sized read also completes when the event ends on a packet boundary.
	BTUSB_QueueEvent( usb, BTUSB_EVENT_ON_DEMAND );

	kind = usb->EventPending;

	error = WaitIO( (PTR) request );

	usb->EventPending = BTUSB_EVENT_IDLE;

	if ( ( kind == BTUSB_EVENT_PREFETCH ) && ( error == USB2Err_Host_Timeout ) && ( ! request->io_Actual ) )
	{
		// A prefetched transfer can expire while playback is waiting for the peer.
		BTUSB_QueueEvent( usb, BTUSB_EVENT_ON_DEMAND );

		error = WaitIO( (PTR) request );

		usb->EventPending = BTUSB_EVENT_IDLE;
	}

	if ( ( error == USB2Err_Host_Timeout ) && ( ! request->io_Actual ) )
	{
		return ( BT_HCI_TRANSPORT_TIMEOUT );
	}

	if ( error )
	{
		return ( error );
	}

	if ( request->io_Actual > usb->Events->BufferSize )
	{
		return ( USB2Err_Host_Overflow );
	}

	*data = request->io_Data;
	*length = request->io_Actual;

	return ( USB2Err_NoError );
}

// --

void
BTUSB_CancelEvent( struct BTUSB *usb )
{
	struct USB2_IORequest *request;

	if ( ! usb->EventPending )
	{
		return;
	}

	request = usb->Events->IORequests[0];

	if ( ! CheckIO( (PTR) request ) )
	{
		AbortIO( (PTR) request );
	}

	WaitIO( (PTR) request );

	usb->EventPending = BTUSB_EVENT_IDLE;
}

// --
