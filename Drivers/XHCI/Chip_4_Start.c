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
XHCI_Chip_Start ( struct USB2_HCDNode * hn )
{
	struct XHCI_State * state;
	struct USBBase * usbbase;

	U32 count;

	state = hn->hn_HCD.XHCI.State;
	usbbase = hn->hn_USBBase;
	XHCI_Sync ();
	PCI_WRITELONG ( state->Operational + XHCI_USBSTS, XHCI_STS_EVENT | XHCI_STS_PORT_CHANGE );
	PCI_WRITELONG ( state->Runtime + XHCI_IMAN, XHCI_IMAN_PENDING | XHCI_IMAN_ENABLE );
	PCI_WRITELONG ( state->Operational + XHCI_USBCMD, XHCI_CMD_RUN | XHCI_CMD_INT_ENABLE );

	for ( count = 0; count < 100; count++ )
	{
		if ( ! ( PCI_READLONG ( state->Operational + XHCI_USBSTS ) & XHCI_STS_HALTED ) )
		{
			return ( TRUE );
		}

		HCD_WAIT_MS ( hn, 1 );
	}

	return ( FALSE );
}

// --
