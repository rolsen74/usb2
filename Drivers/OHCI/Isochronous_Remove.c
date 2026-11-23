
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
	struct USBBase *usbbase;

	usbbase = hn->hn_USBBase;
	previous = hn->hn_HCD.OHCI.Isoc_Head;
	ed = req->req_HCD.OHCI.ED_Header;

	while( ( previous->ped_virt_next ) && ( previous->ped_virt_next != ed ) )
	{
		previous = previous->ped_virt_next;
	}

	if ( ( ! ed ) || ( ! previous->ped_virt_next ) )
	{
		return;
	}

	ed->ed_flags = LE_SWAP32( LE_SWAP32( ed->ed_flags ) | OHCI_ED_SKIP );

	__asm__ volatile ( "sync" ::: "memory" );

	previous->ed_nexted = ed->ed_nexted;
	previous->ped_virt_next = ed->ped_virt_next;

	__asm__ volatile ( "sync" ::: "memory" );

	// Allow a frame already referencing this ED and done writeback to retire.
	HCD_WAIT_MS( hn, 3 );
}

// --
