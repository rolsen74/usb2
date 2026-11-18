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
XHCI_Command ( struct USB2_HCDNode * hn, U32 low, U32 high, U32 status, U32 control, U32 * result )
{
	struct XHCI_State * state;
	struct USBBase * usbbase;

	U32 count;

	state = hn->hn_HCD.XHCI.State;
	usbbase = hn->hn_USBBase;
	state->Command_Done = FALSE;
	state->Command_Physical = XHCI_Ring_Queue ( &state->Commands, low, high, status, control );
	XHCI_Sync ();
	PCI_WRITELONG ( state->Doorbells, 0 );

	for ( count = 0; count < 1000; count++ )
	{
		XHCI_Event_Poll ( hn );

		if ( state->Command_Done )
		{
			if ( result )
			{
				*result = state->Command_Slot;
			}

			return ( state->Command_Completion == XHCI_CC_SUCCESS );
		}

		HCD_WAIT_MS ( hn, 1 );
	}

	XHCI_Chip_Stop ( hn );
	TASK_SIGNAL ( hn->hn_Task->tn_TaskAdr, hn->HCD_Restart_Chip.sig_Signal_Mask );

	return ( FALSE );
}

// --
