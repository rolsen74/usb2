
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "EHCI.h"

// --

SEC_CODE S32
EHCI_Port_Clr_Enable( struct USB2_HCDNode *hn, U32 port )
{
	U32 address;
	U32 value;

	if ( ( ! port ) || ( port > hn->hn_HUB_NumPorts ) )
	{
		return ( USB2Err_Host_HostError );
	}

	address = hn->hn_HCD.EHCI.CapLength + EHCI_PORTSC( port );

	value = PCI_READLONG( address );

	if ( value & EHCI_PS_PO )
	{
		return ( USB2Err_Device_OwnershipChange );
	}

	PCI_WRITELONG( address, value & ~( EHCI_PS_PE | EHCI_PS_CLEAR ) );

	return ( USB2Err_NoError );
}

// --
