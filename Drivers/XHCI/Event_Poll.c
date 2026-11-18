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

SEC_CODE void
XHCI_Event_Poll ( struct USB2_HCDNode * hn )
{
	struct XHCI_State * state;
	struct XHCI_TRB * trb;
	struct RealRequest * req;
	struct XHCI_Buffer * buffer;

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
			for ( req = hn->hn_Active_Transfer_List.uh_Head; req; req = Node_Next ( req ) )
			{
				if ( ( req->req_HCD.XHCI.Slot ) &&
					( req->req_HCD.XHCI.Slot->Slot_ID == slot ) &&
					( req->req_HCD.XHCI.EndPoint == endpoint ) )
				{
					if ( req->req_EndPoint->ep_Type == EPATT_Type_Isochronous )
					{
						for ( buffer = req->req_HCD.XHCI.Buffers; buffer; buffer = buffer->Next )
						{
							if ( buffer->TRB_Physical == physical )
							{
								buffer->Actual = buffer->Length - MIN ( buffer->Length, residual );
								req->req_HCD.XHCI.Actual += buffer->Actual;
								break;
							}
						}

						if ( ( completion != XHCI_CC_SUCCESS ) && ( completion != XHCI_CC_SHORT_PACKET ) )
						{
							req->req_HCD.XHCI.Completion = completion;
						}

						if ( physical == req->req_HCD.XHCI.Last_TRB )
						{
							if ( ! req->req_HCD.XHCI.Completion )
							{
								req->req_HCD.XHCI.Completion = completion;
							}
							req->req_HCD.XHCI.Finished = TRUE;
						}
					}
					else if ( completion == XHCI_CC_SHORT_PACKET )
					{
						req->req_HCD.XHCI.Short_Packet = TRUE;
						for ( buffer = req->req_HCD.XHCI.Buffers; buffer; buffer = buffer->Next )
						{
							if ( buffer->TRB_Physical == physical )
							{
								req->req_HCD.XHCI.Actual = buffer->Offset + buffer->Length - MIN ( buffer->Length, residual );
								break;
							}
						}

						if ( req->req_EndPoint->ep_Type != EPATT_Type_Control )
						{
							req->req_HCD.XHCI.Completion = completion;
							req->req_HCD.XHCI.Finished = TRUE;
						}
					}
					else if ( ( physical == req->req_HCD.XHCI.Last_TRB ) || ( completion != XHCI_CC_SUCCESS ) )
					{
						req->req_HCD.XHCI.Completion = ( ( completion == XHCI_CC_SUCCESS ) &&
							( req->req_HCD.XHCI.Short_Packet ) ) ? XHCI_CC_SHORT_PACKET : completion;
						if ( ( req->req_EndPoint->ep_Type != EPATT_Type_Control ) || ( ! req->req_HCD.XHCI.Short_Packet ) )
						{
							req->req_HCD.XHCI.Actual = req->req_Public.io_Length - MIN ( req->req_Public.io_Length, residual );
						}
						req->req_HCD.XHCI.Finished = TRUE;
					}

					break;
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
