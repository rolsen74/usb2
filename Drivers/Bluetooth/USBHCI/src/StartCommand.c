
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
BTUSB_StartCommand( PTR context, const U8 *command, U32 length )
{
	struct BTUSB *usb;
	struct USB2_EPResource *control;
	struct USB2_IORequest *request;
	struct USB2_IORequest *event;
	struct USB2_SetupData *setup;

	S32 error;

	usb = context;
	control = usb->Registration->Res_Control;
	request = control->IORequests[0];
	setup = control->SetupData;
	event = usb->Events->IORequests[0];

	setup->RequestType = REQTYPE_Write | REQTYPE_Class |
		( usb->InterfaceNumber ? REQTYPE_Interface : REQTYPE_Device );
	setup->RequestCode = 0;
	setup->Value = 0;
	setup->Index = LE_SWAP16( usb->InterfaceNumber );
	setup->Length = LE_SWAP16( length );
	request->io_Command = CMD_WRITE;
	request->io_SetupData = setup;
	request->io_SetupLength = sizeof( *setup );
	request->io_Data = (PTR) command;
	request->io_Length = length;
	request->io_TimeOut = HCI_USB_TIMEOUT;

	event->io_Command = CMD_READ;
	event->io_Data = usb->Events->Buffers[0];
	event->io_Length = usb->Events->BufferSize;
	usb->EventPending = TRUE;

	// Queue the interrupt read before the controller can reply.
	SendIO( (PTR) event );

	error = DoIO( (PTR) request );

	request->io_Data = NULL;

	if ( ( ! error ) && ( request->io_Actual != length ) )
	{
		error = USB2Err_Host_RuntPacket;
	}

	if ( error )
	{
		BTUSB_CancelEvent( usb );
	}

	return ( error );
}

// --
