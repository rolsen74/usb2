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
XHCI_Ring_Queue ( struct XHCI_Ring * ring, U32 low, U32 high, U32 status, U32 control )
{
	struct XHCI_TRB * trb;

	U32 physical;
	U32 index;
	U32 cycle;

	index = ring->Enqueue;
	trb = &ring->TRBs[index];
	physical = ring->Physical + index * sizeof ( struct XHCI_TRB );
	trb->Parameter_Low = LE_SWAP32 ( low );
	trb->Parameter_High = LE_SWAP32 ( high );
	trb->Status = LE_SWAP32 ( status );
	cycle = ring->Cycle;

	if ( ! ring->Unpublished )
	{
		ring->Unpublished = trb;
		cycle ^= XHCI_TRB_CYCLE;
	}

	trb->Control = LE_SWAP32 ( control | cycle );

	index++;

	if ( index == XHCI_RING_TRBS - 1 )
	{
		trb = &ring->TRBs[index];
		trb->Parameter_Low = LE_SWAP32 ( ring->Physical );
		trb->Parameter_High = 0;
		trb->Status = 0;
		trb->Control = LE_SWAP32 ( XHCI_TRB_TYPE ( XHCI_TRB_LINK ) | XHCI_TRB_TOGGLE |
			( control & XHCI_TRB_CHAIN ) | ring->Cycle );
		ring->Cycle ^= 1;
		index = 0;
	}

	ring->Enqueue = index;

	return ( physical );
}

// --
