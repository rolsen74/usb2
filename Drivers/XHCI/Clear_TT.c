
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
XHCI_Clear_TT ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct RealFunctionNode * fn;
	struct RealFunctionNode * hub;
	struct XHCI_Slot * slot;

	U32 type;
	S32 error;

	fn = req->req_Function;
	type = req->req_EndPoint->ep_Type;
	error = req->req_Public.io_Error;

	if ( ( ! hn->hn_AcceptIO ) || ( fn->fkt_Detach ) || ( fn->fkt_HCDGeneration != hn->hn_Generation ) ||
		( fn->fkt_Speed > USBSPEED_Full ) || ( ( type != EPATT_Type_Control ) && ( type != EPATT_Type_Bulk ) ) ||
		( ( error != IOERR_ABORTED ) && ( error != USB2Err_Host_Timeout ) &&
			( error != USB2Err_Host_Overflow ) && ( error != USB2Err_Host_HostError ) &&
			( error != USB2Err_Host_Stall ) ) )
	{
		return ( FALSE );
	}

	hub = fn->fkt_Parent;

	while ( ( hub ) && ( hub->fkt_Tier ) && ( hub->fkt_Speed != USBSPEED_High ) )
	{
		hub = hub->fkt_Parent;
	}

	if ( ( ! hub ) || ( ! hub->fkt_Tier ) || ( hub->fkt_Detach ) )
	{
		return ( FALSE );
	}

	slot = XHCI_Slot_Find ( hn, fn->fkt_NotifyID );

	if ( ( slot ) && ( slot->Address ) && ( HCD_Clear_TT ( hn, req, hub, slot->Address, 1 ) ) )
	{
		return ( TRUE );
	}

	// The hub uses a single TT. Keep this pipe closed if the clear cannot start.
	USB2_HCD_RequestReset ( hn );

	return ( FALSE );
}

// --
