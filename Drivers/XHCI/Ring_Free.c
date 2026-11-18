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
XHCI_Ring_Free ( struct USB2_HCDNode * hn, struct XHCI_Ring * ring )
{
	struct USBBase * usbbase;

	usbbase = hn->hn_USBBase;

	if ( ring->TRBs )
	{
		MEMORY_FREE ( MEMID_HCD_4k, ring->TRBs, ring->Physical );
		ring->TRBs = NULL;
		ring->Physical = 0;
	}
}

// --
