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
XHCI_Transfer_Check ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct XHCI_Ring * ring;
	struct XHCI_Slot * slot;

	U32 completion;
	U32 control;


	if ( ! req->req_HCD.XHCI.Finished )
	{
		return ( FALSE );
	}

	completion = req->req_HCD.XHCI.Completion;

	if ( completion == XHCI_CC_SUCCESS )
	{
		return ( TRUE );
	}

	if ( completion == XHCI_CC_SHORT_PACKET )
	{
		if ( ! req->req_Public.io_AllowShortPackets )
		{
			req->req_Public.io_Error = USB2Err_Host_RuntPacket;
		}

		return ( TRUE );
	}

	if ( completion == 6 )
	{
		req->req_Public.io_Error = USB2Err_Host_Stall;
	}
	else if ( completion == 3 )
	{
		req->req_Public.io_Error = USB2Err_Host_Overflow;
	}
	else
	{
		req->req_Public.io_Error = USB2Err_Host_HostError;
	}

	slot = req->req_HCD.XHCI.Slot;
	ring = &slot->EndPoints[req->req_HCD.XHCI.EndPoint];
	control = ( slot->Slot_ID << 24 ) | ( req->req_HCD.XHCI.EndPoint << 16 );

	if ( ( ! XHCI_Command ( hn, 0, 0, 0, XHCI_TRB_TYPE ( XHCI_TRB_RESET_EP ) | control, NULL ) ) ||
		( ! XHCI_Command ( hn, ( ring->Physical + ring->Enqueue * sizeof ( struct XHCI_TRB ) ) | ring->Cycle,
		0, 0, XHCI_TRB_TYPE ( XHCI_TRB_SET_DEQUEUE ) | control, NULL ) ) )
	{
		USB2_HCD_RequestReset ( hn );
	}

	return ( TRUE );
}

// --
