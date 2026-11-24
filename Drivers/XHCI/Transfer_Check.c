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
	U32 completion;


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

	if ( completion == XHCI_CC_STALL )
	{
		req->req_Public.io_Error = USB2Err_Host_Stall;
	}
	else if ( completion == XHCI_CC_BABBLE )
	{
		req->req_Public.io_Error = USB2Err_Host_Overflow;
	}
	else
	{
		req->req_Public.io_Error = USB2Err_Host_HostError;
	}

	XHCI_Endpoint_Recover ( hn, req, completion );

	return ( TRUE );
}

// --
