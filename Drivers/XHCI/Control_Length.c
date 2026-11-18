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

SEC_CODE U32
XHCI_Control_Length ( struct USB2_HCDNode * hn UNUSED, struct RealRequest * req )
{
	return ( req->req_HCD.XHCI.Actual );
}

// --
