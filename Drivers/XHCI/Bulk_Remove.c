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
XHCI_Bulk_Remove ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	if ( ( req->req_HCD.XHCI.Slot ) && ( ! req->req_HCD.XHCI.Finished ) )
	{
		XHCI_Endpoint_Stop ( hn, req );
	}
}

// --
