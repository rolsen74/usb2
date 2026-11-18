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
XHCI_Port_Set_Reset ( struct USB2_HCDNode * hn, U32 port )
{
	struct XHCI_State * state;
	struct USBBase * usbbase;

	U32 status;
	U32 count;

	state = hn->hn_HCD.XHCI.State;
	usbbase = hn->hn_USBBase;

	if ( ( ! port ) || ( port > state->Max_Ports ) )
	{
		return ( USB2Err_Host_HostError );
	}

	status = PCI_READLONG ( state->Operational + XHCI_PORTSC ( port ) );
	status = XHCI_PORT_WRITE_SAFE ( status );
	PCI_WRITELONG ( state->Operational + XHCI_PORTSC ( port ), status | XHCI_PORT_RESET );

	for ( count = 0; count < 100; count++ )
	{
		HCD_WAIT_MS ( hn, 1 );
		status = PCI_READLONG ( state->Operational + XHCI_PORTSC ( port ) );

		if ( ! ( status & XHCI_PORT_RESET ) )
		{
			state->Port_Reset_Change[port] = TRUE;

			return ( USB2Err_NoError );
		}
	}

	return ( USB2Err_Host_Timeout );
}

// --
