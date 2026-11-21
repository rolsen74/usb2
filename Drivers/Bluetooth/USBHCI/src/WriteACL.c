
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
BTUSB_WriteACL( PTR context, const U8 *data, U32 length )
{
	struct BTUSB *usb;
	struct USB2_IORequest *request;

	U8 *buffer;
	U32 byte;
	S32 error;

	usb = context;

	if ( length > usb->ACLOut->BufferSize )
	{
		return ( USB2Err_Host_Overflow );
	}

	request = usb->ACLOut->IORequests[0];
	buffer = usb->ACLOut->Buffers[0];

	for( byte = 0; byte < length; byte++ )
	{
		buffer[byte] = data[byte];
	}

	request->io_Command = CMD_WRITE;
	request->io_Data = buffer;
	request->io_Length = length;

	error = DoIO( (PTR) request );

	if ( ( ! error ) && ( request->io_Actual != length ) )
	{
		return ( USB2Err_Host_RuntPacket );
	}

	return ( error );
}

// --
