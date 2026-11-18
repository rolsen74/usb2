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
XHCI_Bulk_Add ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct XHCI_State * state;

	state = hn->hn_HCD.XHCI.State;

	if ( ! req->req_HCD.XHCI.Slot )
	{
		return;
	}

	XHCI_Sync ();
	PCI_WRITELONG ( state->Doorbells + req->req_HCD.XHCI.Slot->Slot_ID * 4, req->req_HCD.XHCI.EndPoint );
}

// --
