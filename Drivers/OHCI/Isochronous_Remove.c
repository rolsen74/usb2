
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "OHCI.h"

// --

SEC_CODE void
OHCI_Isochronous_Remove ( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct OHCI_ED *previous;
	struct OHCI_ED *ed;
	previous = hn->hn_HCD.OHCI.Isoc_Head;
	ed = req->req_HCD.OHCI.ED_Header;

	while( ( previous->ped_virt_next ) && ( previous->ped_virt_next != ed ) )
	{
		previous = previous->ped_virt_next;
	}

	if ( ( ! ed ) || ( ! previous->ped_virt_next ) )
	{
		USB2_HCD_RequestReset( hn );

		return;
	}

	OHCI_UnlinkPeriodic( hn, previous, ed );
}

// --
