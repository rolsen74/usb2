
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
XHCI_Ring_Publish ( struct XHCI_Ring * ring )
{
	struct XHCI_TRB * first;

	first = ring->Unpublished;

	if ( first )
	{
		// The rest of the TD must be visible before the controller owns it.
		XHCI_Sync ();

		first->Control ^= LE_SWAP32 ( XHCI_TRB_CYCLE );
		ring->Unpublished = NULL;

		XHCI_Sync ();
	}
}

// --
