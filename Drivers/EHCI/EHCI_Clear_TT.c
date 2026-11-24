
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "EHCI.h"

// --

SEC_CODE S32
EHCI_Clear_TT( struct USB2_HCDNode *hn, struct RealRequest *ioreq )
{
	struct USB2_EndPointNode *ep;
	struct RealFunctionNode *fn;
	struct RealFunctionNode *hub;

	S32 error;

	ep = ioreq->req_EndPoint;
	fn = ioreq->req_Function;
	error = ioreq->req_Public.io_Error;

	if ( ( ! hn->hn_AcceptIO ) || ( fn->fkt_Detach ) || ( fn->fkt_Speed == USBSPEED_High ) ||
		( ( ep->ep_Type != EPATT_Type_Control ) && ( ep->ep_Type != EPATT_Type_Bulk ) ) ||
		( ( error != IOERR_ABORTED ) && ( error != USB2Err_Host_Timeout ) &&
		  ( error != USB2Err_Host_Overflow ) && ( error != USB2Err_Host_UnknownError ) &&
		  ( error != USB2Err_Host_HostError ) ) )
	{
		return ( FALSE );
	}

	hub = fn->fkt_Parent;

	while ( ( hub ) && ( hub->fkt_Speed != USBSPEED_High ) )
	{
		hub = hub->fkt_Parent;
	}

	// The integrated root translator has no hub-class clear request.
	if ( ( ! hub ) || ( ! hub->fkt_Tier ) || ( hub->fkt_Detach ) )
	{
		return ( FALSE );
	}

	// The hub driver uses alternate setting zero, which selects a single TT.
	return ( HCD_Clear_TT( hn, ioreq, hub, fn->fkt_Address, 1 ) );
}

// --
