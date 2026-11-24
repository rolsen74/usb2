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
XHCI_Port_Set_Power ( struct USB2_HCDNode * hn, U32 port )
{
	struct XHCI_State * state;

	U32 status;

	state = hn->hn_HCD.XHCI.State;

	if ( ( ! port ) || ( port > state->Max_Ports ) )
	{
		return ( USB2Err_Host_HostError );
	}

	if ( ! state->Port[port].Major )
	{
		return ( USB2Err_NoError );
	}

	status = PCI_READLONG ( state->Operational + XHCI_PORTSC ( port ) );
	PCI_WRITELONG ( state->Operational + XHCI_PORTSC ( port ), XHCI_PORT_WRITE_SAFE ( status ) | XHCI_PORT_POWER );

	return ( USB2Err_NoError );
}

// --
