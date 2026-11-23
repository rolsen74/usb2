
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

SEC_CODE struct XHCI_Slot *
XHCI_Slot_Find ( struct USB2_HCDNode * hn, USB2_ID function_id )
{
	struct XHCI_State * state;
	struct XHCI_Slot * slot;

	U32 index;

	state = hn->hn_HCD.XHCI.State;

	for ( index = 1; index <= state->Max_Slots; index++ )
	{
		slot = state->Slots[index];

		if ( ( slot ) && ( slot->Function_ID == function_id ) )
		{
			return ( slot );
		}
	}

	return ( NULL );
}

// --

SEC_CODE S32
XHCI_Slot_Is_Retired ( struct USB2_HCDNode * hn, USB2_ID function_id )
{
	return ( ! XHCI_Slot_Find ( hn, function_id ) );
}

// --
