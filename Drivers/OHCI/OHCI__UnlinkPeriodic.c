
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

SEC_CODE S32
OHCI_UnlinkPeriodic( struct USB2_HCDNode *hn, struct OHCI_ED *previous, struct OHCI_ED *ed )
{
	ed->ed_flags = LE_SWAP32( LE_SWAP32( ed->ed_flags ) | OHCI_ED_SKIP );

	__asm__ volatile ( "sync" ::: "memory" );

	previous->ed_nexted = ed->ed_nexted;
	previous->ped_virt_next = ed->ped_virt_next;

	__asm__ volatile ( "sync" ::: "memory" );

	// Each frame loads a new periodic list head. Allow the previous
	// frame and its descriptor writes to finish before freeing this ED.
	// OHCI only publishes the done head. This driver accounts from
	// software-owned TD chains and never walks the hardware done queue.
	if ( ( ! OHCI_WaitFrames( hn ) ) ||
		( ( PCI_READLONG( OHCI_PERIOD_CURRENT_ED ) & OHCI_HEADMASK ) == ed->ped_self_phy ) ||
		( ( PCI_READLONG( OHCI_CONTROL ) & OHCI_HCFS_MASK ) != OHCI_HCFS_OPERATIONAL ) )
	{
		USB2_HCD_RequestReset( hn );

		return ( FALSE );
	}

	__asm__ volatile ( "sync" ::: "memory" );

	return ( TRUE );
}

// --
