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
	U32 reset;
	U32 changed;
	U32 reg;

	state = hn->hn_HCD.XHCI.State;
	usbbase = hn->hn_USBBase;

	if ( ( ! port ) || ( port > state->Max_Ports ) || ( ! state->Port[port].Major ) )
	{
		return ( USB2Err_Host_HostError );
	}

	reg = state->Operational + XHCI_PORTSC ( port );
	reset = ( state->Port[port].Major == 3 ) ? XHCI_PORT_WARM_RESET : XHCI_PORT_RESET;
	changed = ( state->Port[port].Major == 3 ) ? XHCI_PORT_WRC : XHCI_PORT_PRC;
	status = PCI_READLONG ( reg );

	if ( ( ! ( status & XHCI_PORT_CONNECTED ) ) || ( status & XHCI_PORT_CSC ) )
	{
		return ( USB2Err_Device_Detached );
	}

	PCI_WRITELONG ( reg, XHCI_PORT_WRITE_SAFE ( status ) | changed );

	PCI_WRITELONG ( reg, XHCI_PORT_WRITE_SAFE ( status ) | reset );

	for ( count = 0; count < 100; count++ )
	{
		HCD_WAIT_MS ( hn, 1 );
		status = PCI_READLONG ( reg );

		if ( ( ! ( status & XHCI_PORT_CONNECTED ) ) || ( status & XHCI_PORT_CSC ) )
		{
			return ( USB2Err_Device_Detached );
		}

		if ( ( ! ( status & reset ) ) && ( status & changed ) && ( status & XHCI_PORT_ENABLED ) &&
			( ( state->Port[port].Major == 2 ) || ( XHCI_PORT_LINK_STATE ( status ) == 0 ) ) )
		{
			state->Port_Reset_Change[port] = ( reset == XHCI_PORT_RESET );

			return ( USB2Err_NoError );
		}
	}

	return ( USB2Err_Host_Timeout );
}

// --
