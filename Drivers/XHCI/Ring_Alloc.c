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
XHCI_Ring_Alloc ( struct USB2_HCDNode * hn, struct XHCI_Ring * ring )
{
	struct USBBase * usbbase;

	usbbase = hn->hn_USBBase;

	ring->TRBs = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

	if ( ! ring->TRBs )
	{
		return ( FALSE );
	}

	ring->Physical = ( (struct Mem_FreeNode *)ring->TRBs )->mfn_Addr;
	MEM_SET ( ring->TRBs, 0, 4096 );
	ring->Enqueue = 0;
	ring->Dequeue = 0;
	ring->Cycle = 1;
	ring->Consumer_Cycle = 1;

	return ( TRUE );
}

// --
