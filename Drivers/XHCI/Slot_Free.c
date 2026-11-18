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
XHCI_Slot_Free ( struct USB2_HCDNode * hn, struct XHCI_Slot * slot )
{
	struct USBBase * usbbase;
	struct XHCI_State * state;

	U32 index;

	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;

	if ( ! slot )
	{
		return;
	}

	for ( index = 0; index < 32; index++ )
	{
		XHCI_Ring_Free ( hn, &slot->EndPoints[index] );
	}

	if ( slot->Input )
	{
		MEMORY_FREE ( MEMID_HCD_4k, slot->Input, slot->Input_Physical );
	}

	if ( slot->Output )
	{
		MEMORY_FREE ( MEMID_HCD_4k, slot->Output, slot->Output_Physical );
	}

	if ( slot->Slot_ID )
	{
		state->Slots[slot->Slot_ID] = NULL;
	}

	MEM_FREEVEC ( slot );
}

// --
