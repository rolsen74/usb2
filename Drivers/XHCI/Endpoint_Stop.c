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
XHCI_Endpoint_Recover ( struct USB2_HCDNode * hn, struct RealRequest * req, U32 completion )
{
	struct XHCI_Slot * slot;
	struct XHCI_Ring * ring;
	volatile U32 * context;

	U32 dci;
	U32 control;
	U32 state;
	U32 command;

	slot = req->req_HCD.XHCI.Slot;
	dci = req->req_HCD.XHCI.EndPoint;
	ring = &slot->EndPoints[dci];
	control = ( slot->Slot_ID << 24 ) | ( dci << 16 );
	context = (volatile U32 *)( (U8 *)slot->Output + dci * hn->hn_HCD.XHCI.State->Context_Size );

	XHCI_Sync ();

	state = LE_SWAP32 ( *context ) & 7;
	command = 0;

	// The output context can lag the transfer event that changed the state.
	if ( completion == XHCI_CC_TRB_ERROR )
	{
		state = XHCI_EP_ERROR;
	}
	else if ( ( completion == XHCI_CC_STALL ) ||
		( ( req->req_EndPoint->ep_Type != EPATT_Type_Isochronous ) &&
			( ( completion == XHCI_CC_USB_TRANSACTION ) || ( completion == XHCI_CC_BABBLE ) ||
				( completion == XHCI_CC_SPLIT_TRANSACTION ) ) ) )
	{
		state = XHCI_EP_HALTED;
	}

	if ( state == XHCI_EP_HALTED )
	{
		command = XHCI_TRB_RESET_EP;
	}
	else if ( state == XHCI_EP_RUNNING )
	{
		command = XHCI_TRB_STOP_EP;
	}
	else if ( state == XHCI_EP_DISABLED )
	{
		return;
	}
	else if ( ( state != XHCI_EP_STOPPED ) && ( state != XHCI_EP_ERROR ) )
	{
		USB2_HCD_RequestReset ( hn );

		return;
	}

	if ( ( command ) && ( ! XHCI_Command ( hn, 0, 0, 0, XHCI_TRB_TYPE ( command ) | control, NULL ) ) )
	{
		USB2_HCD_RequestReset ( hn );

		return;
	}

	if ( ! XHCI_Command ( hn, ( ring->Physical + ring->Enqueue * sizeof ( struct XHCI_TRB ) ) | ring->Cycle,
		0, 0, XHCI_TRB_TYPE ( XHCI_TRB_SET_DEQUEUE ) | control, NULL ) )
	{
		USB2_HCD_RequestReset ( hn );
	}
}

// --

SEC_CODE void
XHCI_Endpoint_Stop ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	if ( ( req->req_HCD.XHCI.Slot ) && ( req->req_HCD.XHCI.EndPoint ) &&
		( ! req->req_HCD.XHCI.Finished ) )
	{
		XHCI_Endpoint_Recover ( hn, req, 0 );
	}
}

// --
