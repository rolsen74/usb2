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
XHCI_Chip_Deinit ( struct USB2_HCDNode * hn )
{
	struct USBBase * usbbase;
	struct XHCI_State * state;

	U32 * scratch;
	U32 physical;
	U32 index;

	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;

	if ( ! state )
	{
		return;
	}

	for ( index = 1; index <= state->Max_Slots; index++ )
	{
		if ( state->Slots[index] )
		{
			XHCI_Slot_Free ( hn, state->Slots[index] );
		}
	}

	if ( state->Scratchpads )
	{
		scratch = state->Scratchpad_Array;

		for ( index = 0; index < state->Scratchpad_Count; index++ )
		{
			if ( state->Scratchpads[index] )
			{
				physical = LE_SWAP32 ( scratch[index * 2] );
				MEMORY_FREE ( MEMID_HCD_4k, state->Scratchpads[index], physical );
			}
		}

		MEM_FREEVEC ( state->Scratchpads );
		state->Scratchpads = NULL;
	}

	if ( state->Scratchpad_Array )
	{
		MEMORY_FREE ( state->Scratchpad_MemID, state->Scratchpad_Array, state->Scratchpad_Physical );
		state->Scratchpad_Array = NULL;
	}

	if ( state->ERST )
	{
		MEMORY_FREE ( MEMID_HCD_4k, state->ERST, state->ERST_Physical );
		state->ERST = NULL;
	}

	if ( state->DCBAA )
	{
		MEMORY_FREE ( MEMID_HCD_4k, state->DCBAA, state->DCBAA_Physical );
		state->DCBAA = NULL;
	}

	XHCI_Ring_Free ( hn, &state->Events );
	XHCI_Ring_Free ( hn, &state->Commands );
}

// --
