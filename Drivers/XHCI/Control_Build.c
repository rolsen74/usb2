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
XHCI_Control_Build ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct XHCI_State * state;
	struct XHCI_Slot * slot;
	struct XHCI_Ring * ring;
	struct XHCI_Buffer * buffer;
	struct USB2_SetupData * setup;

	U8 * bytes;
	U32 low;
	U32 high;
	U32 count;
	U32 index;
	U32 trt;
	U32 direction;
	U32 status_direction;
	U32 length;
	U32 first;
	U32 td_size;
	U32 packet;

	state = hn->hn_HCD.XHCI.State;
	setup = req->req_Public.io_SetupData;
	length = req->req_Public.io_Length;

	if ( ( ! setup ) || ( req->req_Public.io_SetupLength != 8 ) ||
		( ( length ) && ( ! req->req_Public.io_Data ) ) || ( length > 250 * 4096 ) )
	{
		req->req_Public.io_Error = USB2Err_Stack_InvalidStructure;

		return ( TRUE );
	}

	if ( ( setup->RequestType == ( REQTYPE_Write | REQTYPE_Standard | REQTYPE_Device ) ) &&
		( setup->RequestCode == REQCODE_Set_Address ) && ( ! length ) )
	{
		slot = XHCI_Slot_Create ( hn, req->req_Function );

		if ( ! slot )
		{
			req->req_Public.io_Error = USB2Err_Host_HostError;
		}

		return ( TRUE );
	}

	slot = NULL;

	for ( index = 1; index <= state->Max_Slots; index++ )
	{
		if ( ( state->Slots[index] ) && ( state->Slots[index]->Function == req->req_Function ) )
		{
			slot = state->Slots[index];
			break;
		}
	}

	if ( ! slot )
	{
		req->req_Public.io_Error = USB2Err_Host_HostError;

		return ( TRUE );
	}

	if ( ( req->req_Function->fkt_Speed <= USBSPEED_Full ) &&
		( req->req_EndPoint->ep_MaxPacketSize != slot->EP0_Packet ) )
	{
		if ( ! XHCI_Endpoint_Update0 ( hn, slot, req->req_EndPoint->ep_MaxPacketSize ) )
		{
			req->req_Public.io_Error = USB2Err_Host_HostError;

			return ( TRUE );
		}
	}

	req->req_HCD.XHCI.Slot = slot;
	req->req_HCD.XHCI.EndPoint = 1;
	ring = &slot->EndPoints[1];
	packet = slot->EP0_Packet;
	direction = req->req_Public.io_Command == CMD_READ;
	status_direction = length ? ! direction : 1;
	trt = length ? ( direction ? 3 : 2 ) : 0;
	first = TRUE;

	for ( count = 0; count < length; count += 4096 )
	{
		if ( ! XHCI_Buffer_Add ( hn, req, count, MIN ( 4096, length - count ) ) )
		{
			req->req_Public.io_Error = USB2Err_Stack_NoMemory;

			return ( TRUE );
		}
	}

	bytes = (U8 *)setup;
	low = bytes[0] | ( bytes[1] << 8 ) | ( bytes[2] << 16 ) | ( bytes[3] << 24 );
	high = bytes[4] | ( bytes[5] << 8 ) | ( bytes[6] << 16 ) | ( bytes[7] << 24 );
	XHCI_Ring_Queue ( ring, low, high, 8,
		XHCI_TRB_TYPE ( XHCI_TRB_SETUP ) | XHCI_TRB_IDT | ( trt << 16 ) );

	for ( buffer = req->req_HCD.XHCI.Buffers; buffer; buffer = buffer->Next )
	{
		td_size = buffer->Next ? MIN ( 31, ( length + packet - 1 ) / packet -
			( buffer->Offset + buffer->Length ) / packet ) : 0;
		buffer->TRB_Physical = XHCI_Ring_Queue ( ring, buffer->Physical, 0,
			buffer->Length | ( td_size << 17 ),
			XHCI_TRB_TYPE ( first ? XHCI_TRB_DATA : XHCI_TRB_NORMAL ) |
			( buffer->Next ? XHCI_TRB_CHAIN : 0 ) |
			( ( direction ) && ( first ) ? XHCI_TRB_DIR_IN : 0 ) |
			( direction ? XHCI_TRB_ISP : 0 ) );
		first = FALSE;
	}

	req->req_HCD.XHCI.Last_TRB = XHCI_Ring_Queue ( ring, 0, 0, 0,
		XHCI_TRB_TYPE ( XHCI_TRB_STATUS ) | XHCI_TRB_IOC |
		( status_direction ? XHCI_TRB_DIR_IN : 0 ) );

	return ( FALSE );
}

// --
