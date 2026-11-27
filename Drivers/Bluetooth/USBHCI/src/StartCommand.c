
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static S32
BTUSB_RetireResponse( struct BTUSB *usb, U16 opcode )
{
	const U8 *event;

	U32 length;
	U32 reads;
	S32 error;

	usb->CommandOpcode = opcode;
	usb->ResponsePending = TRUE;

	usb->ResponseDeadline = BTUSB_Now( usb ) + HCI_USB_TIMEOUT;

	for( reads = 0; ( reads < 8 ) && ( usb->UncertainResponse ); reads++ )
	{
		error = BTUSB_ReadEventChunk( usb, & event, & length );

		if ( error )
		{
			return ( error );
		}
	}

	if ( usb->UncertainResponse )
	{
		usb->ResponsePending = FALSE;

		return ( BT_HCI_TRANSPORT_TIMEOUT );
	}

	return ( 0 );
}

// --

S32
BTUSB_StartCommand( PTR context, const U8 *command, U32 length )
{
	struct BTUSB *usb;
	struct USB2_EPResource *control;
	struct USB2_IORequest *request;
	struct USB2_SetupData *setup;

	S32 error;
	U16 opcode;

	usb = context;

	if ( ( ! command ) || ( length < 3 ) || ( length != 3U + command[2] ) )
	{
		return ( USB2Err_Stack_InvalidStructure );
	}

	opcode = command[0] | ( (U16) command[1] << 8 );

	if ( ( usb->UncertainResponse ) && ( opcode == usb->UncertainOpcode ) )
	{
		error = BTUSB_RetireResponse( usb, opcode );

		if ( error )
		{
			return ( error );
		}
	}

	control = usb->Registration->Res_Control;
	request = control->IORequests[0];
	setup = control->SetupData;

	setup->RequestType = REQTYPE_Write | REQTYPE_Class |
		usb->CommandRecipient;
	setup->RequestCode = 0;
	setup->Value = 0;
	setup->Index = ( usb->CommandRecipient == REQTYPE_Interface ) ? LE_SWAP16( usb->InterfaceNumber ) : 0;
	setup->Length = LE_SWAP16( length );
	request->io_Command = CMD_WRITE;
	request->io_SetupData = setup;
	request->io_SetupLength = sizeof( *setup );
	request->io_Data = (PTR) command;
	request->io_Length = length;
	request->io_TimeOut = HCI_USB_TIMEOUT;

	error = BTUSB_Do( usb, request );
	request->io_Data = NULL;

	if ( ( ! error ) && ( request->io_Actual != length ) )
	{
		error = USB2Err_Host_RuntPacket;
	}

	if ( ! error )
	{
		usb->CommandOpcode = opcode;
		usb->ResponsePending = TRUE;

		usb->ResponseDeadline = BTUSB_Now( usb ) +
			( usb->CheckingReadiness ? HCI_READY_TIMEOUT : HCI_USB_TIMEOUT );
	}

	return ( error );
}

// --
