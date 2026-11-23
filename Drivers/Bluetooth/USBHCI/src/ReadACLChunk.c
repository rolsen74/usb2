
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
BTUSB_ReadACLChunk( PTR context, const U8 **data, U32 *length )
{
	struct BTUSB *usb;
	struct USB2_IORequest *request;

	S32 error;

	usb = context;
	request = usb->ACLIn->IORequests[0];
	request->io_Command = CMD_READ;
	request->io_Data = usb->ACLIn->Buffers[0];
	request->io_Length = usb->ACLIn->BufferSize;

	error = BTUSB_Do( usb, request );

	BTUSB_RecoverStall( usb, usb->ACLIn, error );

	if ( ( error == USB2Err_Host_Timeout ) && ( ! request->io_Actual ) )
	{
		return ( BT_HCI_TRANSPORT_TIMEOUT );
	}

	if ( error )
	{
		return ( error );
	}

	if ( request->io_Actual > usb->ACLIn->BufferSize )
	{
		return ( USB2Err_Host_Overflow );
	}

	*data = request->io_Data;
	*length = request->io_Actual;

	return ( USB2Err_NoError );
}

// --
