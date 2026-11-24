
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
XHCI_Port_Clr_Super_Change ( struct USB2_HCDNode * hn, U32 port, U32 feature )
{
	struct XHCI_State * state;

	U32 change;
	U32 reg;
	U32 status;

	state = hn->hn_HCD.XHCI.State;

	if ( ( ! port ) || ( port > state->Max_Ports ) || ( state->Port[port].Major != 3 ) )
	{
		return ( USB2Err_Host_Stall );
	}

	switch ( feature )
	{
		case PORT_CLR_BH_RESET: change = XHCI_PORT_WRC; break;
		case PORT_CLR_LINK_STATE: change = XHCI_PORT_PLC; break;
		case PORT_CLR_CONFIG_ERROR: change = XHCI_PORT_CEC; break;
		default: return ( USB2Err_Host_Stall );
	}

	reg = state->Operational + XHCI_PORTSC ( port );
	status = PCI_READLONG ( reg );

	PCI_WRITELONG ( reg, XHCI_PORT_WRITE_SAFE ( status ) | change );

	return ( USB2Err_NoError );
}

// --
