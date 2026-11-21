
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
BTUSB_ReadEventChunk( PTR context, const U8 **data, U32 *length )
{
	struct BTUSB *usb;
	struct USB2_IORequest *request;

	S32 error;

	usb = context;
	request = usb->Events->IORequests[0];

	// A packet-sized read also completes when the event ends on a packet boundary.
	if ( ! usb->EventPending )
	{
		SendIO( (PTR) request );
	}

	error = WaitIO( (PTR) request );

	usb->EventPending = FALSE;

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

	usb->EventPending = FALSE;
}

// --
