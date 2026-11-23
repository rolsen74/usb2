
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

	U64 now;
	U32 timeout;

	if ( usb->EventPending != BTUSB_EVENT_IDLE )
	{
		return;
	}

	timeout = HCI_USB_TIMEOUT;

	if ( usb->ResponsePending )
	{
		now = BTUSB_Now( usb );

		if ( now >= usb->ResponseDeadline )
		{
			return;
		}

		timeout = usb->ResponseDeadline - now;
	}

	request = usb->Events->IORequests[0];
	request->io_Command = CMD_READ;
	request->io_Data = usb->Events->Buffers[0];
	request->io_Length = usb->Events->BufferSize;
	request->io_TimeOut = timeout;
	usb->EventPending = kind;

	SendIO( (PTR) request );
}

// --

static S32
BTUSB_EventPacket( struct BTUSB *usb )
{
	struct USB2_IORequest *request;
	enum BTUSBEventRead kind;

	S32 error;
	U32 retry;

	request = usb->Events->IORequests[0];

	for( retry = 0; retry < 2; retry++ )
	{
		BTUSB_QueueEvent( usb, BTUSB_EVENT_ON_DEMAND );

		kind = usb->EventPending;

		if ( kind == BTUSB_EVENT_IDLE )
		{
			return ( BT_HCI_TRANSPORT_TIMEOUT );
		}

		error = BTUSB_Wait( usb, request );
		usb->EventPending = BTUSB_EVENT_IDLE;

		if ( ( ( ! error ) || ( error == USB2Err_Host_Timeout ) ) && ( ! request->io_Actual ) )
		{
			if ( kind == BTUSB_EVENT_PREFETCH )
			{
				continue;
			}

			return ( BT_HCI_TRANSPORT_TIMEOUT );
		}

		if ( error )
		{
			BTUSB_RecoverStall( usb, usb->Events, error );

			return ( error );
		}

		if ( request->io_Actual > usb->Events->BufferSize )
		{
			return ( USB2Err_Host_Overflow );
		}

		usb->EventOffset = 0;
		usb->EventBytes = request->io_Actual;

		return ( 0 );
	}

	return ( BT_HCI_TRANSPORT_TIMEOUT );
}

// --

S32
BTUSB_ReadEventChunk( PTR context, const U8 **data, U32 *length )
{
	struct BTUSB *usb;

	U32 reads;
	U16 opcode;
	S32 error;
	const U8 *packet;

	usb = context;
	reads = sizeof( usb->EventFrame );
	error = 0;

	while( usb->FrameUsed < usb->FrameLength )
	{
		error = BTUSB_StopError( usb );

		if ( error )
		{
			break;
		}

		if ( usb->EventOffset == usb->EventBytes )
		{
			if ( ! reads-- )
			{
				error = USB2Err_Host_RuntPacket;

				break;
			}

			error = BTUSB_EventPacket( usb );

			if ( error )
			{
				break;
			}

			continue;
		}

		packet = usb->Events->Buffers[0];
		usb->EventFrame[usb->FrameUsed++] = packet[usb->EventOffset++];

		if ( usb->FrameUsed == 2 )
		{
			usb->FrameLength = 2U + usb->EventFrame[1];
		}
	}

	if ( error )
	{
		if ( usb->ResponsePending )
		{
			if ( error == BT_HCI_TRANSPORT_TIMEOUT )
			{
				usb->UncertainOpcode = usb->CommandOpcode;
				usb->UncertainResponse = TRUE;
			}

			usb->ResponsePending = FALSE;
		}

		if ( ( error == USB2Err_Host_Stall ) || ( error == USB2Err_Host_RuntPacket ) )
		{
			usb->FrameUsed = 0;
			usb->FrameLength = 2;
		}

		return ( error );
	}

	*data = usb->EventFrame;
	*length = usb->FrameLength;
	opcode = 0xffff;

	if ( ( usb->EventFrame[0] == 0x0e ) && ( usb->FrameLength >= 5 ) )
	{
		opcode = usb->EventFrame[3] | ( (U16) usb->EventFrame[4] << 8 );
	}
	else if ( ( usb->EventFrame[0] == 0x0f ) && ( usb->FrameLength == 6 ) )
	{
		opcode = usb->EventFrame[4] | ( (U16) usb->EventFrame[5] << 8 );
	}

	if ( ( usb->ResponsePending ) && ( opcode == usb->CommandOpcode ) )
	{
		usb->ResponsePending = FALSE;
	}

	if ( ( usb->UncertainResponse ) && ( opcode == usb->UncertainOpcode ) )
	{
		usb->UncertainResponse = FALSE;
	}

	usb->FrameUsed = 0;
	usb->FrameLength = 2;

	return ( 0 );
}

// --

void
BTUSB_CancelEvent( struct BTUSB *usb )
{
	struct USB2_IORequest *request;

	usb->EventBytes = 0;
	usb->EventOffset = 0;
	usb->FrameUsed = 0;
	usb->FrameLength = 2;
	usb->ResponsePending = FALSE;

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
