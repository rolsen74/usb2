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
XHCI_Port_Clr_OverCurrent_Chg ( struct USB2_HCDNode * hn, U32 port )
{
	struct XHCI_State * state;

	U32 status;

	state = hn->hn_HCD.XHCI.State;

	if ( ( ! port ) || ( port > state->Max_Ports ) || ( ! state->Port[port].Major ) )
	{
		return ( USB2Err_Host_HostError );
	}

	status = PCI_READLONG ( state->Operational + XHCI_PORTSC ( port ) );
	PCI_WRITELONG ( state->Operational + XHCI_PORTSC ( port ), XHCI_PORT_WRITE_SAFE ( status ) | XHCI_PORT_OCC );

	return ( USB2Err_NoError );
}

// --
