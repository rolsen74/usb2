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
XHCI_Chip_Reset ( struct USB2_HCDNode * hn )
{
	struct XHCI_State * state;
	struct USBBase * usbbase;

	U32 count;
	U32 cmd;

	state = hn->hn_HCD.XHCI.State;
	usbbase = hn->hn_USBBase;
	for ( count = 0; count < 1000; count++ )
	{
		if ( ! ( PCI_READLONG ( state->Operational + XHCI_USBSTS ) & XHCI_STS_NOT_READY ) )
		{
			break;
		}

		HCD_WAIT_MS ( hn, 1 );
	}

	if ( count == 1000 )
	{
		return ( FALSE );
	}

	cmd = PCI_READLONG ( state->Operational + XHCI_USBCMD );
	PCI_WRITELONG ( state->Operational + XHCI_USBCMD, cmd & ~XHCI_CMD_RUN );

	for ( count = 0; count < 100; count++ )
	{
		if ( PCI_READLONG ( state->Operational + XHCI_USBSTS ) & XHCI_STS_HALTED )
		{
			break;
		}
		HCD_WAIT_MS ( hn, 1 );
	}

	if ( count == 100 )
	{
		return ( FALSE );
	}

	PCI_WRITELONG ( state->Operational + XHCI_USBCMD, XHCI_CMD_RESET );

	for ( count = 0; count < 1000; count++ )
	{
		if ( ( ! ( PCI_READLONG ( state->Operational + XHCI_USBCMD ) & XHCI_CMD_RESET ) ) &&
			( ! ( PCI_READLONG ( state->Operational + XHCI_USBSTS ) & XHCI_STS_NOT_READY ) ) )
		{
			return ( TRUE );
		}

		HCD_WAIT_MS ( hn, 1 );
	}

	return ( FALSE );
}

// --
