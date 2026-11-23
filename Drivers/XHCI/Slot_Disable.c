
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
XHCI_Slot_Disable ( struct USB2_HCDNode * hn, struct XHCI_Slot * slot )
{
	struct XHCI_State * state;

	U32 * dcbaa;
	U32 id;

	state = hn->hn_HCD.XHCI.State;
	dcbaa = state->DCBAA;
	id = slot->Slot_ID;

	if ( ! XHCI_Command ( hn, 0, 0, 0, XHCI_TRB_TYPE ( XHCI_TRB_DISABLE_SLOT ) | ( id << 24 ), NULL ) )
	{
		return ( FALSE );
	}

	dcbaa[id * 2] = 0;
	dcbaa[id * 2 + 1] = 0;

	XHCI_Sync ();

	XHCI_Slot_Free ( hn, slot );

	return ( TRUE );
}

// --
