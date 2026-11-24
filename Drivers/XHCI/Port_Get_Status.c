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

SEC_CODE U32
XHCI_Port_Get_Status ( struct USB2_HCDNode * hn, U32 port )
{
	struct XHCI_State * state;

	U32 status;
	U32 result;
	U32 speed;

	state = hn->hn_HCD.XHCI.State;

	if ( ( ! port ) || ( port > state->Max_Ports ) || ( ! state->Port[port].Major ) )
	{
		return ( 0 );
	}

	status = PCI_READLONG ( state->Operational + XHCI_PORTSC ( port ) );
	speed = state->Port[port].Speed[XHCI_PORT_SPEED ( status )];
	result = 0;

	if ( status & XHCI_PORT_CONNECTED )
	{
		result |= HUBF_Status_Connection;
	}

	if ( ( status & XHCI_PORT_ENABLED ) &&
		( ( state->Port[port].Major == 2 ) || ( XHCI_PORT_LINK_STATE ( status ) == 0 ) ) )
	{
		result |= HUBF_Status_Enable;
	}

	if ( status & XHCI_PORT_OVERCURRENT )
	{
		result |= HUBF_Status_Over_Current;
	}

	if ( status & ( XHCI_PORT_RESET | XHCI_PORT_WARM_RESET ) )
	{
		result |= HUBF_Status_Reset;
	}

	if ( status & XHCI_PORT_POWER )
	{
		result |= HUBF_Status_Power;
	}

	if ( speed == USBSPEED_Low + 1 )
	{
		result |= HUBF_Status_Low_Speed;
	}

	if ( speed == USBSPEED_High + 1 )
	{
		result |= HUBF_Status_High_Speed;
	}

	if ( state->Port[port].Major == 3 )
	{
		result |= HUBF_Status_Super_Speed;
	}

	if ( status & XHCI_PORT_CSC )
	{
		result |= HUBF_Chg_Connection;
	}

	if ( status & XHCI_PORT_PEC )
	{
		result |= HUBF_Chg_Enable;
	}

	if ( status & XHCI_PORT_OCC )
	{
		result |= HUBF_Chg_Over_Current;
	}

	if ( ( status & XHCI_PORT_PRC ) || ( state->Port_Reset_Change[port] ) )
	{
		result |= HUBF_Chg_Reset;
	}

	if ( state->Port[port].Major == 3 )
	{
		if ( status & XHCI_PORT_WRC )
		{
			result |= HUBF_Chg_Warm_Reset;
		}

		if ( status & XHCI_PORT_PLC )
		{
			result |= HUBF_Chg_Link_State;
		}

		if ( status & XHCI_PORT_CEC )
		{
			result |= HUBF_Chg_Config_Error;
		}
	}

	return ( result );
}

// --
