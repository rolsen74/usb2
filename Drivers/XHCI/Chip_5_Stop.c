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
XHCI_Chip_Stop ( struct USB2_HCDNode * hn )
{
	struct XHCI_State * state;
	struct USBBase * usbbase;

	U32 count;

	state = hn->hn_HCD.XHCI.State;
	usbbase = hn->hn_USBBase;

	if ( ! state )
	{
		return ( TRUE );
	}

	PCI_WRITELONG ( state->Runtime + XHCI_IMAN, 0 );
	PCI_WRITELONG ( state->Operational + XHCI_USBCMD, 0 );

	for ( count = 0; count < 100; count++ )
	{
		if ( PCI_READLONG ( state->Operational + XHCI_USBSTS ) & XHCI_STS_HALTED )
		{
			return ( TRUE );
		}

		HCD_WAIT_MS ( hn, 1 );
	}

	return ( FALSE );
}

// --
