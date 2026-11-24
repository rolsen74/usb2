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

SEC_CODE static S32
OwnsTRB ( struct RealRequest * req, U32 physical )
{
	struct XHCI_Ring * ring;

	U32 offset;
	U32 first;
	U32 last;

	ring = &req->req_HCD.XHCI.Slot->EndPoints[req->req_HCD.XHCI.EndPoint];
	offset = physical - ring->Physical;

	if ( ( offset >= ( XHCI_RING_TRBS - 1 ) * sizeof ( struct XHCI_TRB ) ) ||
		( offset % sizeof ( struct XHCI_TRB ) ) )
	{
		return ( FALSE );
	}

	first = req->req_HCD.XHCI.First_TRB;
	last = req->req_HCD.XHCI.Last_TRB;

	if ( first <= last )
	{
		return ( ( physical >= first ) && ( physical <= last ) );
	}

	return ( ( physical >= first ) || ( physical <= last ) );
}

// --

SEC_CODE static void
TransferEvent ( struct RealRequest * req, U32 physical, U32 completion, U32 residual )
{
	struct XHCI_Buffer * buffer;

	S32 last;

	last = physical == req->req_HCD.XHCI.Last_TRB;
	buffer = req->req_HCD.XHCI.Buffers;

	while ( ( buffer ) && ( buffer->TRB_Physical != physical ) )
	{
		buffer = buffer->Next;
	}

	if ( req->req_EndPoint->ep_Type == EPATT_Type_Isochronous )
	{
		if ( ( ! buffer ) || ( buffer->Done ) )
		{
			return;
		}

		buffer->Done = TRUE;

		switch ( completion )
		{
			case XHCI_CC_SUCCESS:
			case XHCI_CC_SHORT_PACKET:
				buffer->Actual = buffer->Length - MIN ( buffer->Length, residual );
				req->req_HCD.XHCI.Actual += buffer->Actual;
				break;

			case XHCI_CC_USB_TRANSACTION:
			case XHCI_CC_MISSED_SERVICE:
			case XHCI_CC_ISOCH_BUFFER_OVERRUN:
			case XHCI_CC_SPLIT_TRANSACTION:
				buffer->Failed = TRUE;
				req->req_HCD.XHCI.Iso_Lost = TRUE;
				break;

			default:
				req->req_HCD.XHCI.Completion = completion;
				req->req_HCD.XHCI.Finished = TRUE;

				return;
		}

		if ( last )
		{
			req->req_HCD.XHCI.Completion = XHCI_CC_SUCCESS;
			req->req_HCD.XHCI.Finished = TRUE;
		}

		return;
	}

	if ( completion == XHCI_CC_SHORT_PACKET )
	{
		if ( ( buffer ) && ( ! req->req_HCD.XHCI.Short_Packet ) )
		{
			req->req_HCD.XHCI.Short_Packet = TRUE;
			req->req_HCD.XHCI.Actual = buffer->Offset + buffer->Length - MIN ( buffer->Length, residual );
		}

		if ( ! last )
		{
			return;
		}
	}
	else if ( completion == XHCI_CC_SUCCESS )
	{
		if ( ! last )
		{
			return;
		}

		if ( req->req_HCD.XHCI.Short_Packet )
		{
			completion = XHCI_CC_SHORT_PACKET;
		}
		else
		{
			req->req_HCD.XHCI.Actual = req->req_Public.io_Length - MIN ( req->req_Public.io_Length, residual );
		}
	}

	req->req_HCD.XHCI.Completion = completion;
	req->req_HCD.XHCI.Finished = TRUE;
}

// --

SEC_CODE void
XHCI_Event_Poll ( struct USB2_HCDNode * hn )
{
	struct XHCI_State * state;
	volatile struct XHCI_TRB * trb;
	struct RealRequest * req;

	U32 control;
	U32 status;
	U32 physical;
	U32 type;
	U32 slot;
	U32 endpoint;
	U32 completion;
	U32 residual;

	state = hn->hn_HCD.XHCI.State;
	XHCI_Sync ();

	while ( TRUE )
	{
		trb = &state->Events.TRBs[state->Events.Dequeue];
		control = LE_SWAP32 ( trb->Control );

		if ( ( control & XHCI_TRB_CYCLE ) != state->Events.Consumer_Cycle )
		{
			break;
		}

		XHCI_Sync ();

		status = LE_SWAP32 ( trb->Status );
		physical = LE_SWAP32 ( trb->Parameter_Low );
		type = XHCI_TRB_GET_TYPE ( control );
		slot = control >> 24;
		endpoint = ( control >> 16 ) & 0x1f;
		completion = status >> 24;
		residual = status & 0xffffff;

		if ( type == XHCI_TRB_COMMAND_EVENT )
		{
			if ( physical == state->Command_Physical )
			{
				state->Command_Completion = status >> 24;
				state->Command_Slot = slot;
				state->Command_Done = TRUE;
			}
		}
		else if ( type == XHCI_TRB_TRANSFER_EVENT )
		{
			if ( ( completion == XHCI_CC_RING_UNDERRUN ) || ( completion == XHCI_CC_RING_OVERRUN ) )
			{
				state->Iso_Empty_Ring++;
			}
			else
			{
				if ( completion == XHCI_CC_MISSED_SERVICE )
				{
					state->Iso_Missed_Service++;
				}

				for ( req = hn->hn_Active_Transfer_List.uh_Head; req; req = Node_Next ( req ) )
				{
					if ( ( req->req_HCD.XHCI.Slot ) && ( ! req->req_HCD.XHCI.Finished ) &&
						( req->req_HCD.XHCI.Slot->Slot_ID == slot ) &&
						( req->req_HCD.XHCI.EndPoint == endpoint ) && ( OwnsTRB ( req, physical ) ) )
					{
						TransferEvent ( req, physical, completion, residual );

						break;
					}
				}
			}
		}

		state->Events.Dequeue++;

		if ( state->Events.Dequeue == XHCI_RING_TRBS )
		{
			state->Events.Dequeue = 0;
			state->Events.Consumer_Cycle ^= 1;
		}
	}

	XHCI_Sync ();
	PCI_WRITELONG ( state->Runtime + XHCI_ERDP,
		( state->Events.Physical + state->Events.Dequeue * sizeof ( struct XHCI_TRB ) ) | XHCI_ERDP_BUSY );
}

// --
